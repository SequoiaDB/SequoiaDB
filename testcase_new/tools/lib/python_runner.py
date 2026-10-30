# -*- coding: utf-8 -*-
"""统一执行器的环境管理：配置副本、节点 before/after 与 S3 服务生命周期。

临时 cwd 用于兼容旧配置加载器，与用例配置中的 WORKDIR 无关。
恢复失败时保留 node.conf.ini 或远程 S3 备份，供后续恢复使用。
SSH/第三方驱动依赖仅在对应类型实际运行时加载。
"""
from __future__ import print_function
import io
import json
import os
import subprocess
import tempfile
import threading
from contextlib import contextmanager

# ROOT 为 testcase_new，用于定位公共 JS；不依赖调用时 cwd。
ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
CONFIGS = {'driver': ('SDB_CONFIG', 'config.json'), 's3': ('S3_CONFIG', 's3tests.json'),
           'fapmongo': ('FAP_CONFIG', 'config.json')}


CONF_KEYS = ('DB_HOSTNAME', 'SVCNAME', 'CHANGEDPREFIX', 'RSRVPORTBEGIN', 'RSRVPORTEND', 'RSRVNODEDIR',
             'S3HOSTNAME', 'S3SVCNAME', 'ROOT_USER', 'ROOT_PWD', 'INSTALL_DIR')


def environment_conf(path):
    """读取与其他语言共用的 KEY=value 配置；同名环境变量优先，空值视为未设置。"""
    conf = {}
    if path:
        with io.open(path, encoding='utf-8') as stream:
            for line in stream:
                line = line.strip()
                if line and not line.startswith('#') and '=' in line:
                    key, value = line.split('=', 1)
                    conf[key.strip()] = value.strip()
    values = {}
    for key in CONF_KEYS:
        value = os.environ.get(key) or conf.get(key)
        if value:
            values[key] = value
    return values


def number(value):
    return int(value) if value is not None and str(value).isdigit() else value


def read_config(args, case_dir):
    """读取用例配置，依次用 --conf 和命令行覆盖，返回独立的配置字典。"""
    key, name = CONFIGS[args.type]
    with io.open(os.environ.get(key, os.path.join(case_dir, name)), encoding='utf-8') as stream:
        data = json.load(stream)
    if not isinstance(data, dict):
        raise ValueError('configuration must be a JSON object')
    env = args.env
    host = args.host or env.get('DB_HOSTNAME')
    port = str(args.port) if args.port is not None else env.get('SVCNAME')
    if args.type == 'driver':
        updates = (('HOSTNAME', host), ('SVCNAME', port), ('CHANGEDPREFIX', env.get('CHANGEDPREFIX')),
                   ('RSRVPORTBEGIN', number(env.get('RSRVPORTBEGIN'))), ('RSRVPORTEND', number(env.get('RSRVPORTEND'))),
                   ('RSRVNODEDIR', args.db_path or env.get('RSRVNODEDIR')))
    elif args.type == 'fapmongo':
        # HOSTNAME/SVCNAME 供 changeConf 连 COORD；FAPHOST/FAPPORT 仍只来自用例配置。
        updates = (('HOSTNAME', host), ('SVCNAME', port), ('CHANGEDPREFIX', env.get('CHANGEDPREFIX')),)
    else:
        coord = None
        if host or port:
            old_host, old_port = (data.get('sdb_coord_urls') or 'localhost:11810').split(',')[0].rsplit(':', 1)
            coord = '%s:%s' % (host or old_host, port or old_port)
        updates = (('sdb_coord_urls', coord), ('host', env.get('S3HOSTNAME')), ('remote_host', env.get('S3HOSTNAME')),
                   ('port', number(env.get('S3SVCNAME'))), ('remote_user', env.get('ROOT_USER')),
                   ('remote_password', env.get('ROOT_PWD')))
    # 未显式传参时保留源配置 WORKDIR，不创建或清空该目录。
    data.update((key, value) for key, value in updates + (('WORKDIR', args.workdir),) if value is not None)
    if args.type == 'driver':
        data.setdefault('HOSTNAME', 'localhost')
        data.setdefault('SVCNAME', '11810')
        data.setdefault('RSRVNODEDIR', os.path.join(args.install_path, 'database') + os.sep)
    elif args.type == 'fapmongo':
        # changeConf 连 COORD，不能回落到 FAPPORT。
        data.setdefault('HOSTNAME', 'localhost')
        data.setdefault('SVCNAME', '11810')
    return data


@contextmanager
def execution_directory(kind, config):
    """生成本次 JSON 副本，退出时恢复 cwd 并删除私有目录。"""
    # 旧加载器固定从 cwd 读 JSON；切换私有目录，源配置保持原样。
    previous = os.getcwd()
    directory = tempfile.mkdtemp(prefix='runtest_python_')
    path = os.path.join(directory, CONFIGS[kind][1])
    try:
        fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
        with os.fdopen(fd, 'w') as stream:
            json.dump(config, stream)
        os.chdir(directory)
        yield directory
    finally:
        os.chdir(previous)
        import shutil
        shutil.rmtree(directory)


def error_report(report_dir, label, message):
    """将环境准备或恢复失败写成 JUnit 错误，供本地和 CI 识别。"""
    import xml.etree.ElementTree as ET
    root = ET.Element('testsuite', name=label, tests='1', failures='0', errors='1', skipped='0')
    case = ET.SubElement(root, 'testcase', classname='configure', name=label, time='0')
    ET.SubElement(case, 'error', message=label).text = message
    ET.ElementTree(root).write(os.path.join(report_dir, label + '.xml'), encoding='utf-8', xml_declaration=True)


def change_conf(conf, phase, config, report_dir):
    """执行公共节点配置脚本，失败时记录报告并返回 False。"""
    script = os.path.join(ROOT, 'tools', 'changeConf.js')
    sdb = os.path.join(os.environ.get('INSTALL_DIR', '/opt/sequoiadb'), 'bin', 'sdb')
    if not os.path.isfile(sdb) or not os.path.isfile(script):
        raise ValueError('configuration requires sdb and changeConf.js')
    host = config.get('HOSTNAME', config.get('FAPHOST', 'localhost'))
    port = config.get('SVCNAME', config.get('FAPPORT', '11810'))
    command = [sdb, '-f', conf + ',' + script, '-e',
               'var clusterOnly=true;var hostname=%s;var svcname=%s;var mode=%s' % (json.dumps(host), json.dumps(str(port)), json.dumps(phase))]
    print('changeconf ' + phase + ': ' + conf)
    proc = subprocess.Popen(command, cwd=os.path.dirname(conf), stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    # 配置脚本可能重启节点；超时或中断时结束子进程，再尝试恢复。
    timer = threading.Timer(600, proc.kill)
    timer.start()
    try:
        output = proc.communicate()[0].decode('utf-8', 'replace')
    except BaseException:
        proc.kill()
        proc.wait()
        raise
    finally:
        timer.cancel()
    print(output)
    if proc.returncode:
        label = 'configure_%s_%s' % (phase, os.path.basename(os.path.dirname(conf)))
        error_report(report_dir, label, 'return code %s\n%s' % (proc.returncode, output))
    return proc.returncode == 0


def normalize_fap_backup(ini):
    """补全备份中的 FAP 关闭状态，保留已记录的配置值。"""
    # 公共脚本会省略 undefined；补空字符串才能恢复到 FAP 关闭状态。
    with io.open(ini, encoding='utf-8') as stream:
        lines = stream.readlines()
    for index, line in enumerate(lines):
        key, separator, value = line.partition('=')
        if separator and key.strip() in ('coordConf', 'standaloneConf'):
            data = json.loads(value)
            data.setdefault('fap', '')
            lines[index] = key + '=' + json.dumps(data) + u'\n'
    with io.open(ini, 'w', encoding='utf-8') as stream:
        stream.writelines(lines)


@contextmanager
def configured(conf, config, report_dir, fap=False):
    """执行 before/after；拒绝覆盖遗留备份，恢复成功后删除备份。"""
    if not conf:
        yield True
        return
    ini = conf + '.ini'
    if os.path.exists(ini):
        raise ValueError('configuration backup already exists; restore it first: ' + ini)
    try:
        yield change_conf(conf, 'before', config, report_dir)
    finally:
        if os.path.isfile(ini):
            if fap:
                normalize_fap_backup(ini)
            # 只有恢复成功才删除备份，失败时保留恢复依据。
            if change_conf(ini, 'after', config, report_dir):
                os.remove(ini)


def case_conf(path, case_dir):
    """从用例目录向上查找 node.conf，查找范围止于用例根目录。"""
    base = os.path.dirname(path)
    while base.startswith(case_dir):
        conf = os.path.join(base, 'node.conf')
        if os.path.isfile(conf):
            return conf
        if base == case_dir:
            break
        base = os.path.dirname(base)
    return None

def quote(value):
    """对远程 shell 的单个参数加引号，防止路径或配置值被解释为命令。"""
    return "'" + str(value).replace("'", "'\"'\"'") + "'"


class S3Environment(object):
    """准备 S3 后交给用例执行，退出时逆序清理已登记的资源。"""
    def __init__(self, config, case_dir):
        self.config, self.case_dir = config, case_dir
        self.cleanup = []

    def command(self, command, allow_stopped=False):
        """执行远程命令并检查退出码，失败时中止当前阶段。"""
        stdin, stdout, stderr = self.ssh.exec_command(command)
        output = stdout.read().decode('utf-8', 'replace')
        rc = stdout.channel.recv_exit_status()
        # 仅 status 明确报告没有进程时接受非零状态；其他 SSH 错误仍抛出。
        if rc != 0 and not (allow_stopped and any(line.strip() == 'Total: 0' for line in output.splitlines())):
            raise RuntimeError('S3 remote command failed: %s; %s' % (command, stderr.read()))
        return output

    def upload(self, local, remote):
        self.sftp.put(local, remote)

    def start(self):
        self.stop()
        java_home = self.config.get('remote_java_home')
        java_env = '' if not java_home else 'export JAVA_HOME=%s; export PATH=$JAVA_HOME/bin:$PATH; ' % quote(java_home)
        for attempt in range(3):
            self.command('source /etc/profile; %s%s start >> /tmp/s3start.log' % (java_env, quote(self.service)))
            if 'Total: 0' not in self.command(quote(self.service) + ' status', allow_stopped=True):
                return
        raise RuntimeError('S3 start failed; see /tmp/s3start.log and sequoias3.log')

    def stop(self):
        for attempt in range(3):
            if 'Total: 0' in self.command(quote(self.service) + ' status', allow_stopped=True):
                return
            self.command(quote(self.service) + ' stop -a')
        if 'Total: 0' not in self.command(quote(self.service) + ' status', allow_stopped=True):
            raise RuntimeError('S3 stop failed; see sequoias3.log')

    def configure_service(self):
        """先备份原配置，成功后追加本次测试的 S3 参数。"""
        self.command('cp -p %s %s' % (quote(self.properties), quote(self.backup)))
        self.cleanup.append(self.restore_service)
        settings = 'server.port=%s%ssdbs3.sequoiadb.url=sequoiadb://%s%ssdbs3.multipartupload.completereservetime=1%s' % (
            self.config['port'], chr(10), self.config['sdb_coord_urls'], chr(10), chr(10))
        self.command("printf '%s' %s >> %s" % ('%s', quote(settings), quote(self.properties)))

    def restore_service(self):
        self.command('mv %s %s' % (quote(self.backup), quote(self.properties)))

    def update_database(self, phase):
        """上传并执行既有 JS，设置或清理 S3 用例所需数据库参数。"""
        import json
        name = 'updateSdbConf.js' if phase == 'before' else 'deleteSdbConf.js'
        remote = self.remote_dir + '/' + name
        self.upload(os.path.join(self.case_dir, 'script', name), remote)
        host, port = self.config['sdb_coord_urls'].split(',')[0].rsplit(':', 1)
        values = 'var hostname=%s;var port=%s;var config=%s' % (
            json.dumps(host), json.dumps(port), json.dumps(json.dumps(dict(transisolation=1, translockwait=True))))
        print(self.command('%s -f %s -e %s' % (quote(self.install + '/bin/sdb'), quote(remote), quote(values))))

    def _cleanup(self, failed):
        errors = []
        # 单个清理动作失败也继续其他动作，并保留原始执行异常。
        for action in reversed(self.cleanup):
            try:
                action()
            except Exception as error:
                errors.append(str(error))
        self.cleanup = []
        if errors:
            message = 'S3 cleanup failed: ' + '; '.join(errors)
            if failed:
                print(message)
            else:
                raise RuntimeError(message)

    def __enter__(self):
        import paramiko
        try:
            self.ssh = paramiko.SSHClient()
            self.cleanup.append(self.ssh.close)
            self.ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
            self.ssh.connect(hostname=self.config['remote_host'], port=22,
                             username=self.config['remote_user'], password=self.config['remote_password'])
            self.sftp = self.ssh.open_sftp()
            self.cleanup.append(self.sftp.close)
            self.install = self.command("awk -F= '/^INSTALL_DIR=/{print $2}' /etc/default/sequoiadb").strip()
            if not self.install:
                raise RuntimeError('S3 INSTALL_DIR not found')
            self.service = self.install + '/tools/sequoias3/sequoias3.sh'
            self.properties = self.install + '/tools/sequoias3/config/application.properties'
            self.backup = self.install + '/tools/sequoias3/config/ori_application.properties'
            self.command('test ! -e ' + quote(self.backup))
            self.remote_dir = self.command('mktemp -d /tmp/runtest_python_s3.XXXXXX').strip()
            if not self.remote_dir.startswith('/tmp/runtest_python_s3.') or '/' in self.remote_dir[len('/tmp/'):]:
                raise RuntimeError('S3 remote temporary directory is invalid')
            self.cleanup.append(lambda: self.command('rm -f %s %s && rmdir %s' % (
                quote(self.remote_dir + '/updateSdbConf.js'), quote(self.remote_dir + '/deleteSdbConf.js'), quote(self.remote_dir))))
            # 修改环境前登记恢复动作，覆盖准备过程执行到一半的失败。
            self.cleanup.append(lambda: self.update_database('after'))
            self.update_database('before')
            self.configure_service()
            self.cleanup.append(self.stop)
            self.start()
            return True
        except BaseException:
            self._cleanup(True)
            raise

    def __exit__(self, error_type, error, trace):
        self._cleanup(error_type is not None)
        return False

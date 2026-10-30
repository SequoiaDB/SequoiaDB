#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""本地与 CI 的统一 Python 用例入口，兼容 Python 2.7 和 Python 3。

示例：python testcase_new/tools/runtest_python.py -t driver -m normal
      python testcase_new/tools/runtest_python.py -t s3
依赖：XML runner；三类用例各自的驱动/第三方依赖由调用方准备。
负责参数、筛选、执行、日志和报告；环境准备由 python_runner 管理。
不调用三个旧 run_all_case.py，不修改旧加载器和仓库 JSON 配置。
"""
from __future__ import print_function
import argparse
import fnmatch
import glob
import io
import itertools
import os
import re
import shutil
import signal
import sys
import time
import traceback
import unittest
import xml.etree.ElementTree as ET

# 用例也有 lib 包；直接导入辅助文件，避免争用 lib 包名。
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), 'lib'))
from python_runner import (read_config, environment_conf, execution_directory, configured, case_conf, error_report,
                           quote, S3Environment)

# 默认目录由脚本位置推导，与调用时 cwd 和自定义报告目录无关。
ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
CASES = dict(driver=os.path.join(ROOT, 'driver', 'python'),
             s3=os.path.join(ROOT, 'thirdparty', 's3', 'python', 's3-tests-master'),
             fapmongo=os.path.join(ROOT, 'thirdparty', 'fapmongo', 'python', 'fap3-pymongo3.10.1'))
MODE_LISTS = dict(sync='sync_list', configure='configure_list', largedata='large_data_list', reliability='reliability_list')
MODES = dict(driver=('normal', 'sync', 'configure', 'all'), s3=('configure', 'all'), fapmongo=('configure', 'all'))
DEFAULT_CONF = os.path.join(ROOT, 'tools', 'conf', 'test.conf')
USAGE = 'python testcase_new/tools/runtest_python.py [-p path]|[-f file] [-t type] [-m mode] [options]'
HELP = u'''Usage:
  python testcase_new/tools/runtest_python.py [-p path]|[-f file] [-t type] [-m mode] [options]

 -p path                  : 运行指定目录下的用例，相对用例根目录，如 connect；与 -f 互斥
 -f file                  : 运行指定的用例文件，相对用例根目录，可重复，如 connect/test_connect_12501.py；与 -p 互斥
 -t type                  : 运行指定类型的用例，可取 driver|s3|fapmongo，默认 driver
 -m mode                  : 运行指定模式的用例，driver 默认 normal，s3、fapmongo 默认 configure
                             - driver 可取 normal|sync|configure|all
                             - s3、fapmongo 可取 configure|all
                             - normal：基本功能用例；sync：集群拓扑用例，会增删节点
                             - configure：配置类用例，会修改集群或服务配置
                             - all：该类型下的全部用例
 -s stopFlag              : 发生用例错误是否停止，0 表示继续，1 表示停止，默认为 0
 -h hostname              : 指定 COORD 节点 HostName 或 IP，默认 localhost
 -n svcname               : 指定 COORD 节点服务名，默认 11810
 -print [failed]          : 屏幕打印方式，不指定时每个用例只打印一行结果
                             - failed：额外打印失败用例的错误信息和调用栈
                             - 不带参数：打印 unittest 原始输出
 --dry-run                : 只打印将执行的用例，不执行
 --report-dir dir         : 指定报告目录，默认 local_test_report/python/<type>/<mode>/python<主版本>/

 --conf file              : 环境配置文件，默认 testcase_new/tools/conf/test.conf
 --install-dir dir        : 指定 SequoiaDB 安装目录，默认 /opt/sequoiadb
 --db-path dir            : 指定用例新建节点的数据目录，默认 <install-dir>/database/
 --case-dir dir           : 指定用例根目录，默认 testcase_new/driver/python
                             - s3：testcase_new/thirdparty/s3/python/s3-tests-master
                             - fapmongo：testcase_new/thirdparty/fapmongo/python/fap3-pymongo3.10.1
 --workdir dir            : 指定用例临时文件目录，默认取配置文件中的 WORKDIR
 --reportFormat jenkins   : 将报告改写为 Jenkins 格式，CI 使用
 --reportinfo kv          : CI 报告信息，格式 k=v;k=v，可取 execute_host|cluster_hosts|diaglog_url|
                            diaglog_base|test_type|deploy_mode|build_number
                             - 未指定 diaglog_url 时按 <diaglog_base>/<test_type>_<deploy_mode>_<build_number>.tar.gz 拼接
 --help                   : 显示帮助
'''
RESULT_COLORS = {'pass': '\033[32m', 'skip': '\033[33m', 'fail': '\033[31m'}
RESULT_LABELS = {'pass': '[PASS]', 'skip': '[SKIP]', 'fail': '[FAIL]'}
RESULT_WIDTH = 20


def terminal_width(stream):
    if hasattr(shutil, 'get_terminal_size'):
        return shutil.get_terminal_size((120, 20)).columns
    try:
        return int(os.environ.get('COLUMNS', 120))
    except (TypeError, ValueError):
        return 120


def result_text(status, seconds, stream):
    plain = '%s %7.3fs' % (RESULT_LABELS[status], seconds)
    value = '%*s' % (RESULT_WIDTH, plain)
    if hasattr(stream, 'isatty') and stream.isatty() and 'NO_COLOR' not in os.environ:
        value = RESULT_COLORS[status] + value + '\033[0m'
    return value


def short_case_name(test_id):
    class_name, method = test_id.split('#', 1)
    module_name = class_name.rsplit('.', 1)[0].rsplit('.', 1)[-1]
    return '%s#%s' % (module_name, method)


def case_detail(report_dir, test_id):
    class_name, method = test_id.split('#', 1)
    parts = [re.sub(r'[^\w-]', '_', part) for part in class_name.split('.')]
    if parts[0] == 'python':
        parts.pop(0)
    return os.path.join(report_dir, 'cases', *(parts + [re.sub(r'[^\w-]', '_', method) + '_out.txt']))


def result_line(name, status, seconds, stream):
    columns = terminal_width(stream)
    left_width = max(1, columns - RESULT_WIDTH - 2)
    if len(name) > left_width:
        name = name[:max(1, left_width - 3)] + '...'
    return '%-*s  %s' % (left_width, name, result_text(status, seconds, stream))


# 参数解析
def main(argv=None):
    parser = argparse.ArgumentParser(add_help=False, usage=USAGE)
    selection = parser.add_mutually_exclusive_group()
    selection.add_argument('-p', dest='directory')
    selection.add_argument('-f', dest='files', action='append', default=[])
    parser.add_argument('-t', dest='type', default='driver', choices=sorted(CASES))
    parser.add_argument('-m', dest='mode')
    parser.add_argument('-s', dest='stop', default='0', choices=('0', '1'))
    parser.add_argument('-h', dest='host')
    parser.add_argument('-n', dest='port', type=int)
    # 不加 -print 为 None：逐用例一行；-print 不带值为 raw：原样输出 unittest 日志。
    parser.add_argument('-print', dest='print_mode', nargs='?', const=True, default=None, choices=('failed',))
    parser.add_argument('--dry-run', action='store_true')
    parser.add_argument('--report-dir')
    parser.add_argument('--conf')
    parser.add_argument('--install-dir')
    parser.add_argument('--db-path')
    parser.add_argument('--case-dir')
    parser.add_argument('--workdir')
    parser.add_argument('--reportFormat', choices=('jenkins',))
    parser.add_argument('--reportinfo', default='')
    parser.add_argument('--help', action='store_true')

    args = parser.parse_args(argv)
    if args.help:
        sys.stdout.write(HELP if sys.version_info[0] > 2 else HELP.encode('utf-8'))
        return 0
    args.mode = args.mode or ('normal' if args.type == 'driver' else 'configure')
    if args.mode not in MODES[args.type]:
        parser.error('%s 可取的模式为 %s' % (args.type, '|'.join(MODES[args.type])))
    if args.port is not None and not 1 <= args.port <= 65535:
        parser.error('-n 必须在 1 到 65535 之间')
    if args.type == 'fapmongo' and (args.host is not None or args.port is not None):
        parser.error('-h、-n 只适用于 driver 和 s3')
    if args.type != 'driver' and args.db_path is not None:
        parser.error('--db-path 只适用于 driver')
    if args.type == 's3' and (args.workdir is not None or args.install_dir is not None):
        parser.error('--install-dir、--workdir 只适用于 driver 和 fapmongo')
    if args.install_dir is not None:
        args.install_dir = os.path.abspath(args.install_dir)
    if args.print_mode is True:
        args.print_mode = 'raw'
    args.stop = args.stop == '1'
    args.printed = set()
    return execute(args)


# 用例发现与名单筛选
def resolve_case_path(case_dir, name):
    path = os.path.normpath(name)
    # 仓库路径按明确的 testcase_new 前缀识别，不依赖调用时的工作目录。
    if path == 'testcase_new' or path.startswith('testcase_new' + os.sep):
        return os.path.abspath(os.path.join(os.path.dirname(ROOT), path))
    return os.path.abspath(os.path.join(case_dir, path))


def select_cases(case_dir, kind, mode, explicit, report_dir, directory=None):
    """返回排序后的用例文件；-f/-p 优先，all 跳过模式名单过滤。"""
    def inside_case_dir(path):
        relative = os.path.relpath(os.path.realpath(path), os.path.realpath(case_dir))
        return relative != os.pardir and not relative.startswith(os.pardir + os.sep)
    # 指定 -f 后只检查文件有效性，不再应用模式名单。
    if explicit:
        files = sorted(set(resolve_case_path(case_dir, name) for name in explicit))
        for path in files:
            if not os.path.isfile(path) or not path.endswith('.py'):
                raise ValueError('test file not found or not Python: ' + path)
            if not inside_case_dir(path):
                raise ValueError('test file must be inside case-dir: ' + path)
        return files
    search_dir = case_dir
    if directory is not None:
        search_dir = resolve_case_path(case_dir, directory)
        if not inside_case_dir(search_dir):
            raise ValueError('test directory must be inside case-dir: ' + directory)
        if not os.path.isdir(search_dir):
            raise ValueError('test directory not found: ' + search_dir)
    lists = {}
    config = os.path.join(case_dir, 'testcase.conf')
    if kind == 'fapmongo':
        config = os.path.join(os.path.dirname(case_dir), 'testcase.conf')
    if kind != 's3' and os.path.isfile(config):
        with io.open(config, encoding='utf-8') as stream:
            for line in stream:
                if '=' in line and not line.lstrip().startswith('#'):
                    key, value = line.split('=', 1)
                    lists[key.strip()] = [x.strip() for x in value.strip().split(',') if x.strip()]
    common = set(lists.get('common_list', [])) | set(('lib', '__pycache__', 'report', 'local_test_report', 'output', '.git'))
    common.update(('__init__.py', 'run_all_case.py', 'config.py', 'utils.py'))
    special = set(x for key in MODE_LISTS.values() for x in lists.get(key, []))
    selected = set(lists.get(MODE_LISTS.get(mode), []))
    files = []
    pattern = 'test_*_support.py' if kind == 's3' else '*.py'
    for base, dirs, names in os.walk(search_dir):
        if directory is not None and (os.path.realpath(base) == os.path.realpath(report_dir) or any(
                part in common or part.startswith('.')
                for part in os.path.relpath(base, case_dir).split(os.sep) if part != os.curdir)):
            dirs[:] = []
            continue
        dirs[:] = sorted(d for d in dirs if d not in common and not d.startswith('.')
                         and (directory is None or inside_case_dir(os.path.join(base, d)))
                         and os.path.realpath(os.path.join(base, d)) != os.path.realpath(report_dir))
        for name in sorted(names):
            if name in common or not fnmatch.fnmatch(name, pattern):
                continue
            path = os.path.join(base, name)
            if directory is not None and not inside_case_dir(path):
                raise ValueError('test file must be inside case-dir: ' + path)
            parts = os.path.relpath(path, case_dir).split(os.sep)
            entries = set(parts + [os.path.basename(case_dir), name[:-3]])
            # 显式目录/all 仍排除公共文件；没有名单的目录直接发现用例。
            if kind != 's3' and lists and mode != 'all' and directory is None:
                if mode == 'normal' and entries & special:
                    continue
                if mode != 'normal' and not entries & selected:
                    continue
            files.append(path)
    return sorted(files)


# 日志保存与终端输出
class Log(object):
    """原样保存输出，并按需同步到终端。"""
    def __init__(self, stream, mirror=None):
        self.stream, self.mirror = stream, mirror

    def write(self, text):
        if isinstance(text, bytes):
            text = text.decode('utf-8', 'replace')
        self.stream.write(text)
        if self.mirror is not None:
            try:
                self.mirror.write(text)
            except UnicodeEncodeError:
                self.mirror.write(text.encode('utf-8', 'replace'))
            self.mirror.flush()
        self.stream.flush()

    def flush(self):
        self.stream.flush()

    def finish(self):
        self.flush()


# 用例执行与中断处理
class Interrupted(KeyboardInterrupt):
    def __init__(self, code):
        self.code = code


def interrupt(signum, frame):
    """将中断转为可清理的异常，退出码为 128 加信号编号。"""
    # 首次中断转为异常；忽略后续信号，让 finally 完成环境恢复。
    signal.signal(signal.SIGINT, signal.SIG_IGN)
    signal.signal(signal.SIGTERM, signal.SIG_IGN)
    raise Interrupted(128 + signum)


def run_cases(args, case_dir, files, report_dir, config, log, console):
    """准备类型环境后执行 unittest，返回实际执行的测试方法数量。"""
    import xmlrunner
    class Runner(xmlrunner.XMLTestRunner):
        def run(self, suite):
            self.case_output = {}
            result = super(Runner, self).run(suite)
            # 旧 xmlrunner 把整批输出复制到每个 XML；按方法替换，类级日志留在完整日志中。
            for path in glob.glob(os.path.join(report_dir, 'TEST-*.xml')):
                tree = ET.parse(path)
                root = tree.getroot()
                for case in root.findall('testcase'):
                    output = self.case_output.get(case.get('classname', '') + '.' + case.get('name', ''))
                    if output is None:
                        continue
                    for tag, text in zip(('system-out', 'system-err'), output):
                        for node in case.findall(tag):
                            case.remove(node)
                        if text:
                            ET.SubElement(case, tag).text = text
                for node in root.findall('system-out') + root.findall('system-err'):
                    root.remove(node)
                tree.write(path, encoding='utf-8', xml_declaration=True)
            return result

        def _make_result(self):
            result = super(Runner, self)._make_result()
            start_test = result.startTest
            stop_test = result.stopTest
            def captured(stream):
                # xmlrunner 2.x 用 _DuplicateWriter 转发，缓冲在 _second。
                if hasattr(stream, 'getvalue'):
                    return stream.getvalue()
                second = getattr(stream, '_second', None)
                if second is not None and hasattr(second, 'getvalue'):
                    return second.getvalue()
                return ''
            def start(test):
                start_test(test)
                self.output_offsets = (len(captured(sys.stdout)), len(captured(sys.stderr)))
            def stop(test):
                self.case_output[test.id()] = (captured(sys.stdout)[self.output_offsets[0]:],
                                              captured(sys.stderr)[self.output_offsets[1]:])
                stop_test(test)
                bad = any(case.id() == test.id() for case, error in result.failures + result.errors)
                bad = bad or any(case.id() == test.id() for case in result.unexpectedSuccesses)
                skip = any(case.id() == test.id() for case, reason in result.skipped)
                # unittest 使用点分隔方法名，报告显示统一使用 #。
                test_id = '#'.join(test.id().rsplit('.', 1))
                name = short_case_name(test_id)
                status = 'fail' if bad else 'skip' if skip else 'pass'
                # -print 原样输出时不打结果行；-print failed 在失败行后附上错误和详情文件。
                if args.print_mode != 'raw':
                    args.printed.add(test_id)
                    print(result_line(name, status, result.stop_time - result.start_time, console),
                          file=console)
                    if args.print_mode == 'failed' and status == 'fail':
                        out, err = self.case_output.get(test.id(), ('', ''))
                        traces = [error for case, error in result.failures + result.errors if case.id() == test.id()]
                        detail = (out or '') + (err or '') + '\n'.join(traces)
                        if detail and not detail.endswith('\n'):
                            detail += '\n'
                        detail += '    detail : ' + os.path.relpath(case_detail(report_dir, test_id), report_dir) + '\n'
                        try:
                            console.write(detail)
                        except UnicodeEncodeError:
                            console.write(detail.encode('utf-8', 'replace'))
                    console.flush()
            result.stopTest = stop
            result.startTest = start
            return result
    # driver 的 ssl/ 与标准库同名；从父目录按包导入，避免遮蔽标准库。
    top = os.path.dirname(case_dir) if args.type == 'driver' and os.path.isfile(os.path.join(case_dir, '__init__.py')) else case_dir
    sys.path.insert(0, top)
    if args.type == 's3':
        context = S3Environment(config, case_dir)
    else:
        conf = os.path.join(os.path.dirname(case_dir), 'node.conf') if args.type == 'fapmongo' else None
        if conf and not os.path.isfile(conf):
            raise ValueError('FAPMongo node.conf not found: ' + conf)
        context = configured(conf, config, report_dir, fap=args.type == 'fapmongo')
    with context as ready:
        if ready is False:
            return 0
        expected = 0
        # 只合并相邻的同配置文件，保持普通用例与配置用例的排序顺序。
        for conf, paths in itertools.groupby(files, lambda path: case_conf(path, case_dir) if args.type == 'driver' else None):
            with configured(conf, config, report_dir) as ready:
                if not ready:
                    if args.stop:
                        break
                    continue
                suite = unittest.TestSuite()
                loader = unittest.TestLoader()
                for path in paths:
                    base = os.path.dirname(path)
                    discovery_top = top
                    if not os.path.isfile(os.path.join(base, '__init__.py')):
                        discovery_top = base
                        if args.type == 'driver':
                            sys.path.append(base)
                        else:
                            sys.path.insert(0, base)
                    __import__(os.path.relpath(path, discovery_top)[:-3].replace(os.sep, '.'))
                    suite.addTests(loader.discover(base, pattern=os.path.basename(path), top_level_dir=discovery_top))
                result = Runner(output=report_dir, stream=log, failfast=args.stop).run(suite)
                expected += result.testsRun
                if args.stop and (result.failures or result.errors or result.unexpectedSuccesses):
                    break
    return expected


# 报告汇总
def summarize(report_dir, emit, args, case_dir, console):
    """汇总 XML 并写失败清单，返回总数、失败数、跳过数和有效用例结果数。"""
    total = failed = skipped = 0
    failures = []
    cases, documents, reported = {}, [], set()
    for path in sorted(glob.glob(os.path.join(report_dir, 'TEST-*.xml')) + glob.glob(os.path.join(report_dir, 'configure_*.xml'))):
        tree = ET.parse(path)
        root = tree.getroot()
        if root.tag not in ('testsuite', 'testsuites'):
            raise ValueError('invalid JUnit root: ' + path)
        documents.append((path, tree))
        for suite in root.iter('testsuite'):
            for case in list(suite.findall('testcase')):
                fixture = re.match(r'^(setUpClass|tearDownClass|setUpModule|tearDownModule) \((.+)\)$',
                                   case.get('name', ''))
                if fixture:
                    case.set('classname', fixture.group(2))
                    case.set('name', fixture.group(1))
                if not case.get('classname') or not case.get('name'):
                    raise ValueError('JUnit testcase missing classname/name: ' + path)
                name = case.get('classname') + '#' + case.get('name')
                is_fixture = fixture is not None or case.get('classname') == 'unittest.suite._ErrorHolder'
                if os.path.basename(path).startswith('TEST-') and not is_fixture:
                    reported.add(name)
                if name in cases:
                    original = cases[name][0]
                    original.set('time', str(max(float(original.get('time', '0')), float(case.get('time', '0')))))
                    for child in case:
                        if child.tag in ('failure', 'error', 'skipped') or not any(
                                child.tag == previous.tag and child.text == previous.text for previous in original):
                            original.append(child)
                    suite.remove(case)
                else:
                    cases[name] = [case]
    # 同一方法可在正文和 tearDown 多次报错；JUnit 只计一次，诊断全部保留。
    for records in cases.values():
        case = records[0]
        errors = case.findall('error') + case.findall('failure')
        if errors:
            diagnostic = u'\n\n'.join(u'%s: %s\n%s' % (node.get('type', node.tag), node.get('message', ''), node.text or '')
                                      for node in errors)
            outcome = next((node for node in errors if node.tag == 'error'), errors[0])
            for node in errors + case.findall('skipped'):
                case.remove(node)
            outcome.text = diagnostic
            case.append(outcome)
    for path, tree in documents:
        for suite in reversed(list(tree.iter('testsuite'))):
            direct = suite.findall('testcase')
            children = suite.findall('testsuite')
            for key, value in (('tests', len(direct)),
                               ('errors', sum(case.find('error') is not None for case in direct)),
                               ('failures', sum(case.find('failure') is not None for case in direct)),
                               ('skipped', sum(case.find('skipped') is not None for case in direct))):
                suite.set(key, str(value + sum(int(child.get(key, '0')) for child in children)))
            suite.set('time', str(sum(float(case.get('time', '0')) for case in direct)
                                  + sum(float(child.get('time', '0')) for child in children)))
        if tree.getroot().tag == 'testsuites':
            for key in ('tests', 'errors', 'failures', 'skipped'):
                tree.getroot().set(key, str(sum(int(suite.get(key, '0')) for suite in tree.getroot().findall('testsuite'))))
            tree.getroot().set('time', str(sum(float(suite.get('time', '0')) for suite in tree.getroot().findall('testsuite'))))
        tree.write(path, encoding='utf-8', xml_declaration=True)
    rows = []
    for name, records in sorted(cases.items()):
        errors = [node for case in records for node in case.findall('failure') + case.findall('error')]
        bad = bool(errors)
        skip = not bad and any(case.find('skipped') is not None for case in records)
        total += 1
        failed += int(bad)
        skipped += int(skip)
        seconds = max(float(case.get('time', '0')) for case in records)
        status = 'fail' if bad else 'skip' if skip else 'pass'
        class_name, method = name.split('#', 1)
        detail = case_detail(report_dir, name)
        if not os.path.isdir(os.path.dirname(detail)):
            os.makedirs(os.path.dirname(detail))
        command = '%s %s -t %s -m %s --case-dir %s' % (quote(sys.executable), quote(os.path.abspath(__file__)),
                                                        args.type, args.mode, quote(case_dir))
        module_name = class_name if method in ('setUpModule', 'tearDownModule') else class_name.rsplit('.', 1)[0]
        module = sys.modules.get(module_name)
        if module is not None and hasattr(module, '__file__'):
            command += ' -f ' + quote(os.path.relpath(module.__file__, case_dir).replace('.pyc', '.py').replace('.pyo', '.py'))
        for option, value in (('--conf', args.conf), ('--db-path', args.db_path), ('--workdir', args.workdir),
                              ('--install-dir', args.install_dir)):
            if value is not None:
                command += ' ' + option + ' ' + quote(value)
        if args.host is not None:
            command += ' -h ' + quote(args.host)
        if args.port is not None:
            command += ' -n ' + str(args.port)
        with io.open(detail, 'w', encoding='utf-8') as stream:
            stream.write(u'CMD: %s\ncase   : %s\nstatus : %s\ntime   : %.3fs\nlog    : runtest_python.log\n\n' %
                         (command, name, status, seconds))
            for node in errors:
                stream.write((node.text or node.get('message', '')) + u'\n')
            for case in records:
                for node in case.findall('system-out') + case.findall('system-err'):
                    stream.write((node.text or '') + u'\n')
        if bad:
            failures.append(name)
        rows.append((name, short_case_name(name), status, seconds, detail if bad else None))
    if rows:
        columns = terminal_width(console)
        width = max(1, columns - RESULT_WIDTH - 2)
        emit('%-*s  %*s' % (width, 'test case', RESULT_WIDTH, 'result time'), display=False)
        emit('%s  %s' % ('-' * width, '-' * RESULT_WIDTH), display=False)
        for name, display_name, status, seconds, detail in rows:
            plain_line = result_line(display_name, status, seconds, None)
            # 执行中已打印的用例不再重复；类级错误、配置修改失败等只在这里补打。
            show = args.print_mode != 'raw' and name not in args.printed
            use_color = show and hasattr(console, 'isatty') and console.isatty() and 'NO_COLOR' not in os.environ
            emit(plain_line, display=show and not use_color)
            if use_color:
                print(result_line(display_name, status, seconds, console), file=console)
            if detail:
                emit('    detail : ' + os.path.relpath(detail, report_dir), display=show and args.print_mode == 'failed')
    if failures:
        with io.open(os.path.join(report_dir, 'failed_cases.txt'), 'w', encoding='utf-8') as stream:
            stream.write(u'\n'.join(failures) + u'\n')
    return total, failed, skipped, len(reported)
# 完整执行流程
def execute(args):
    """依次解析目录、筛选、配置、执行和汇总，退出时恢复进程状态。

    返回：0 成功；1 用例失败、缺少有效结果或报告异常；2 参数/配置/依赖/环境错误；
    3 没有用例；130/143 对应 SIGINT/SIGTERM。
    """
    case_dir = os.path.abspath(args.case_dir or CASES[args.type])
    report_dir = os.path.abspath(args.report_dir or os.path.join(
        ROOT, '..', 'local_test_report', 'python', args.type, args.mode, 'python%d' % sys.version_info[0]))
    try:
        if not os.path.isdir(case_dir):
            raise ValueError('case directory not found: ' + case_dir)
        case_rel = os.path.relpath(case_dir, report_dir)
        if case_rel == os.curdir or not case_rel.startswith(os.pardir + os.sep):
            raise ValueError('report directory must not contain case directory: ' + report_dir)
        files = select_cases(case_dir, args.type, args.mode, args.files, report_dir, args.directory)
        conf_file = args.conf or (DEFAULT_CONF if os.path.isfile(DEFAULT_CONF) else None)
        args.env = environment_conf(conf_file)
        # 安装目录：/opt/sequoiadb < --conf < 环境变量 INSTALL_DIR < --install-dir。
        args.install_path = args.install_dir or os.path.abspath(args.env.get('INSTALL_DIR', '/opt/sequoiadb'))
        config = read_config(args, case_dir)
    except (IOError, OSError, ValueError) as error:
        print(str(error), file=sys.stderr)
        return 2
    console = sys.stdout
    started = time.time()
    report_display = report_dir if args.report_dir else os.path.relpath(report_dir, os.path.dirname(ROOT))
    print('======> Begin Python test: %s/%s python%d =====>' % (args.type, args.mode, sys.version_info[0]))
    print(' Test case : %s (%d files)' % (args.mode, len(files)))
    print(' Case dir  : ' + (case_dir if args.case_dir else os.path.relpath(case_dir, os.path.dirname(ROOT))))
    if args.type == 'driver':
        print(' COORD     : %s:%s' % (config['HOSTNAME'], config['SVCNAME']))
    elif args.type == 's3':
        print(' COORD     : ' + config.get('sdb_coord_urls', ''))
    print(' Log       : ' + os.path.join(report_display, 'runtest_python.log'))
    print(' Report    : ' + report_display)
    print()
    # dry-run 不创建报告或临时配置，也不触发环境准备。
    if args.dry_run:
        print('Interpreter: ' + sys.executable)
        for path in files:
            print('unittest discover: ' + os.path.relpath(path, case_dir))
        print('Selected files: %d%s' % (len(files), '' if files else ' (empty)'))
        return 0 if files else 3
    if not files:
        print('No test case selected', file=sys.stderr)
        return 3
    old_cwd, old_env, old_path = os.getcwd(), os.environ.copy(), sys.path[:]
    old_stdout, old_stderr = sys.stdout, sys.stderr
    handlers = dict((sig, signal.getsignal(sig)) for sig in (signal.SIGINT, signal.SIGTERM))
    rc, expected = 0, 0
    old_bytecode = sys.dont_write_bytecode
    try:
        os.environ['INSTALL_DIR'] = args.install_path
        if not os.path.isdir(report_dir):
            os.makedirs(report_dir)
        # 只清理本执行器的约定产物，保留调用方目录中的其他文件。
        for pattern in ('TEST-*.xml', 'configure_*.xml', 'failed_cases.txt', 'result.txt', 'runtest_python.log'):
            for path in glob.glob(os.path.join(report_dir, pattern)):
                os.remove(path)
        if os.path.isdir(os.path.join(report_dir, 'cases')):
            shutil.rmtree(os.path.join(report_dir, 'cases'))
        with io.open(os.path.join(report_dir, 'runtest_python.log'), 'w', encoding='utf-8') as stream:
            # -print 不带值时把原始输出同步到屏幕，CI 抓的就是这份日志。
            mirror = console if args.print_mode == 'raw' else None
            log = Log(stream, mirror=mirror)
            try:
                sys.stdout = sys.stderr = log
                for sig in handlers:
                    signal.signal(sig, interrupt)
                # driver 仍需顶层 lib 导入，但必须让标准库 ssl 优先于用例 ssl 目录。
                if args.type == 'driver':
                    sys.path.append(case_dir)
                else:
                    sys.path.insert(0, case_dir)
                sys.dont_write_bytecode = True
                # 私有 cwd 存放配置副本；用例代码仍从源目录导入。
                with execution_directory(args.type, config):
                    expected = run_cases(args, case_dir, files, report_dir, config, log, console)
            except Interrupted as error:
                rc = error.code
                print('interrupted: %d' % rc)
            except ET.ParseError:
                rc = 1
                traceback.print_exc()
                print('Python runner report error; see log', file=console)
            except Exception:
                rc = 2
                diagnostic = traceback.format_exc()
                print(diagnostic)
                print('Python runner environment/configuration error; see log', file=console)
                try:
                    error_report(report_dir, 'configure_environment', diagnostic)
                except (IOError, OSError) as report_error:
                    print('Cannot write environment report: ' + str(report_error), file=console)
            finally:
                sys.stdout, sys.stderr = old_stdout, old_stderr
                log.finish()
            with io.open(os.path.join(report_dir, 'result.txt'), 'w', encoding='utf-8') as result:
                def emit(line, display=True):
                    if display:
                        print(line, file=console)
                    result.write(line + u'\n')
                total = failed = skipped = None
                try:
                    total, failed, skipped, reported = summarize(report_dir, emit, args, case_dir, console)
                    # 选中的文件里没有用例返回 3；有用例却缺少结果按失败处理。
                    if rc == 0:
                        rc = 3 if not total and not expected else 1 if failed or reported < expected else 0
                    if args.reportFormat:
                        sys.path.insert(0, os.path.join(ROOT, 'tools', 'report-formatters'))
                        from python_jenkins import format_reports
                        format_reports(report_dir, args.type, args.reportinfo)
                except Exception:
                    traceback.print_exc(file=log)
                    log.finish()
                    rc = rc or 1
                    emit(' report error : report processing failed; exit code %d; see runtest_python.log' % rc)
                ended = time.time()
                emit('\n<====== Finish Python test ======\n')
                emit('***********************************************************')
                emit('                    ***test result***')
                emit(' begin time : ' + time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(started)))
                emit(' end time   : ' + time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(ended)))
                emit(' use time   : %.3f(s)' % (ended - started))
                if total is None:
                    emit(' counts     : unavailable (report processing failed)')
                else:
                    for key, value in (('total', total), ('succeed', total - failed - skipped), ('failed', failed), ('skipped', skipped)):
                        emit(' %-10s : %d' % (key, value))
                emit(' log        : ' + os.path.join(report_display, 'runtest_python.log'))
                emit(' report     : ' + report_display)
                emit('***********************************************************')
    except Interrupted as error:
        rc = error.code
    except (IOError, OSError) as error:
        print(str(error), file=console)
        rc = rc if rc in (130, 143) else 2
    finally:
        # 恢复进程状态，包含用例修改的环境变量和信号处理器。
        os.chdir(old_cwd)
        os.environ.clear()
        os.environ.update(old_env)
        sys.path[:] = old_path
        for sig, handler in handlers.items():
            signal.signal(sig, handler)
        sys.dont_write_bytecode = old_bytecode
    return rc

if __name__ == '__main__':
    sys.exit(main())

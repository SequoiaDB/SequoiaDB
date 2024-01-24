#! /usr/bin/python
# -*- coding: utf-8 -*-
# @Author Nie Zhibiao

# Copyright (c) 2018, SequoiaDB and/or its affiliates. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA

import sys
import os
import re
import time
import socket
import shutil
import signal
import optparse
from sdbaudit_exprt import CryptoUtil, pid_exist
from subprocess import Popen, PIPE
from version import get_version, get_release, get_git_version, get_build_time
try:
    import ConfigParser as ConfigParser
except Exception:
    import configparser as ConfigParser
    import chardet

try:
    reload(sys)
    sys.setdefaultencoding('utf8')
except NameError:
    import importlib,sys
    importlib.reload(sys)

DESCRIPTION = "%prog is a utility to initialize, start, stop, or control a " \
              "sdbaudit server."

USAGE = '''%prog add -t <sdb|mysql|mariadb> [--inst=INSTNAME] [--path=INSTALL_DIR]
       %prog del -t <sdb|mysql|mariadb> [--inst=INSTNAME] [--path=INSTALL_DIR]
       %prog list
       %prog start
       %prog stop
       %prog --version
       %prog --help'''

KW_SSL = 'ssl'
KW_USER = 'user'
KW_ADDR = 'addr'
KW_ROLE = 'role'
KW_DELETE = 'delete'
KW_CL_NAME = 'clname'
#KW_CIPHER = 'cipher'
#KW_TOKEN = 'token'
#KW_CIPHERFILE = 'cipherfile'
KW_PASSWD = 'password'
KW_MONITOR = 'options'
KW_NODE_NAME = 'nodename'
KW_INST_NAME = 'instname'
KW_INSERT_NUM = 'insertnum'
KW_AUDIT_PATH = 'auditpath'
KW_INSTALL_DIR = 'installdir'
KW_PASSWD_TYPE = 'password_type'

LOG_TYPE_SDB = 'sdb'
LOG_TYPE_MYSQL = 'mysql'
LOG_TYPE_MARIADB = 'mariadb'
PORT = 'port'
AUDIT_PATH = 'audit_path'
ROLE = 'role'
INSTALL_DIR = 'install_dir'
INST_NAME = 'inst_name'
ADD_OPERATION = 'add'
DEL_OPERATION = 'del'
LIST_OPERATION = 'list'
STOP_OPERATION = 'stop'
START_OPERATION = 'start'
REGISTER_CONF_PATH = "/etc/default/"
CONFIG_FILE_NAME = "sdbaudit.conf"
PID_FILE_NAME = "sdbaudit.pid"
LOG_FILE_NAME = "sdbaudit.log"
VERSION_FILE_NAME = "version.info"
DAEMON_PID_FILE_NAME = "sdbaudit_daemon.pid"

PROCESS_WAIT_TIME = 5
FORMAT_NUM = 5
MYSQL_DEFAULT_PORT = 3306
MARIADB_DEFAULT_PORT = 6101

MY_HOME = os.path.abspath(os.path.dirname(__file__))
MY_CONF_PATH = os.path.join(MY_HOME, 'conf')
EXPORTER_EXEC = os.path.join(MY_HOME, "sdbaudit_exprt.py")


class OptionsMgr:
    def __init__(self, program):
        self.__parser = optparse.OptionParser(
            description = DESCRIPTION,
            usage = USAGE,
            prog = program
        )
        self.operation = None
        self.log_type = None
        self.instance_name = None
        self.install_dir = None

    def __show_version(self):
        print("sdbaudit: {}".format(get_version())) 
        print("Release: {}".format(get_release())) 
        print("Git version: {}".format(get_git_version())) 
        print(get_build_time())
        sys.exit(0)

    def __add_option(self):
        #Add -t option
        self.__parser.add_option("-t", "--type", action='store',
                                 type="string", dest='log_type', help="audit " \
                                 "log type. Option: sdb, mysql, mariadb")
        #Add --inst option
        self.__parser.add_option("--inst", action='store', type="string",
                                 dest='instance_name', help="instance name. " \
                                 "It can be specified when log type is " \
                                 "mysql or mariadb")
        #Add --path option
        self.__parser.add_option("--path", action='store', type="string",
                                 dest='install_dir', help="installation path. If " \
                                 "not specified, Monitor the audit log files " \
                                 "under the registered path")
        #Add --version option
        self.__parser.add_option("-v", "--version", action='store_true',
                                 dest="version", help="show version")

    def __parse_option(self):
        self.__add_option()
        opt,args = self.__parser.parse_args()
        if not opt.log_type:
            print("[ERROR] Log type can not be null. Please use -t to specify.")
            return 1
        self.log_type = opt.log_type.lower()
        if self.log_type != LOG_TYPE_SDB and self.log_type != LOG_TYPE_MYSQL \
                                  and self.log_type != LOG_TYPE_MARIADB:
            print("[ERROR] Invalid log type, Please use 'sdb','mysql' or " \
                  "'mariadb' instead.")
            return 1
        self.instance_name = opt.instance_name
        if opt.install_dir and (not os.path.isdir(opt.install_dir)):
            print("[ERROR] --path '{}' is not a directory".format(opt.install_dir))
            return 1
        self.install_dir = opt.install_dir
        return 0

    def parse_operation(self):
        if len(sys.argv) < 2:
            print("[ERROR] Invalid argument. Try 'python {} -h' for more " \
                  "information".format(sys.argv[0]))
            return 1
        self.operation = sys.argv[1]
        if self.operation == ADD_OPERATION or self.operation == DEL_OPERATION:
            return self.__parse_option()
        elif self.operation == LIST_OPERATION or self.operation == START_OPERATION \
                                    or self.operation == STOP_OPERATION:
            if len(sys.argv) > 2:
                print("[ERROR] Invalid argument: '{}'. Try 'python {} -h' for more " \
                "information".format(sys.argv[2], sys.argv[0]))
                return 1
        elif self.operation == '--help' or self.operation == '-h':
            self.__add_option()
            self.__parser.parse_args()
        elif self.operation == '--version' or self.operation == '-v':
            self.__show_version()
        else:
            print("[ERROR] Invalid argument: '{}'. Try 'python {} -h' for more " \
                  "information".format(self.operation, sys.argv[0]))
            return 1
        return 0

class ObjMgr:
    def __init__(self, optMgr = None):
        self.__options = {
            KW_MONITOR: {
                KW_USER : '',
                KW_PASSWD : '',
                KW_PASSWD_TYPE : 0,
                KW_ADDR : 'localhost:11810',
                #KW_CIPHER : 'false',
                #KW_TOKEN : '',
                #KW_CIPHERFILE : './passwd',
                KW_INSTALL_DIR : '',
                KW_AUDIT_PATH : '',
                KW_SSL : 'false',
                KW_DELETE : 'true',
                KW_INSERT_NUM : 1000,
                KW_CL_NAME : 'SDBSYS_GLOBAL_AUDIT_LOG_CS.AUDIT_CL',
                KW_ROLE : '',
                KW_NODE_NAME : '',
                KW_INST_NAME : ''
            }
        }
        self.__log_type = optMgr.log_type
        self.__operate = optMgr.operation
        self.__instance_name = optMgr.instance_name
        self.__install_dir = optMgr.install_dir
        self.__info = []

    def __create_work_dir(self):
        if 0 == len(self.__info):
            return 0
        work_dir = os.path.join(MY_CONF_PATH, self.__log_type)
        if not os.path.exists(MY_CONF_PATH):
            os.mkdir(conf_dir)
        if not os.path.exists(work_dir):
            os.mkdir(work_dir)
        local_parser = ConfigParser.ConfigParser()
        local_parser.add_section(KW_MONITOR)
        for i in range(len(self.__info)):
            node_name = str(self.__info[i][PORT])
            obj_dir = os.path.join(work_dir, self.__info[i][PORT])
            if os.path.exists(obj_dir):
                print("Node({}) of {} already exists".format(node_name, self.__log_type))
                continue
            os.mkdir(obj_dir)
            os.chdir(obj_dir)
            local_parser.set(KW_MONITOR, KW_ROLE,
                                     str(self.__info[i][ROLE]))
            if self.__log_type != LOG_TYPE_SDB:
                local_parser.set(KW_MONITOR, KW_INST_NAME,
                                         str(self.__info[i][INST_NAME]))
            local_parser.set(KW_MONITOR, KW_NODE_NAME, socket.gethostname() + \
                             ":" + node_name)
            local_parser.set(KW_MONITOR, KW_INSTALL_DIR, str(self.__info[i][INSTALL_DIR]))
            local_parser.set(KW_MONITOR, KW_AUDIT_PATH, str(self.__info[i][AUDIT_PATH]))
            local_parser.write(open(CONFIG_FILE_NAME, "w+"))
            print("Added node({}) of {} successfully".format(node_name,
                                                             self.__log_type))
        return 0

    def __parse_sdb_info(self):
        if not self.__install_dir:
            conf = os.path.join(REGISTER_CONF_PATH, 'sequoiadb')
            if not os.path.exists(conf):
                print("[ERROR] Register configuration file '{}' does not " \
                      "exist".format(conf))
                return 1
            with open(conf, 'r') as f:
                content = f.read()
                install_conf = re.findall(r'INSTALL_DIR[^\r\n]+', content)[0]
            self.__install_dir = install_conf.split("=")[1].strip()
        command = os.path.join(self.__install_dir, 'bin/sdblist')
        try:
            p = Popen([command, '-t', 'all', '-m', 'local', '--expand'], stdout=PIPE)
        except OSError:
            print("[ERROR] --path '{}' is not the installation directory for " \
                  "{}".format(self.__install_dir, self.__log_type))
            return 1

        buf = p.stdout.read()
        if str != type(buf):
            encode_type = chardet.detect(buf)
            buf = buf.decode(encode_type['encoding'])
        svc_name = [x.replace(' ', '') for x in re.findall(r'svcname[^\r\n]+', buf)]
        audit_path = [x.replace(' ', '') for x in re.findall(r'auditpath[^\r\n]+', buf)]
        role = [x.replace(' ', '') for x in re.findall(r'role[^\r\n]+', buf)]
        for i in range(len(svc_name)):
            tmp = {}
            tmp[PORT] = svc_name[i].split(':')[1].strip()
            tmp[AUDIT_PATH] = audit_path[i].split(':')[1].strip()
            tmp[ROLE] = role[i].split(':')[1].strip()
            tmp[INSTALL_DIR] = self.__install_dir
            self.__info.append(tmp)
        return 0

    def __parse_sql_inst_info(self):
        list_conf = []
        data_dir = []
        inst_name = []
        install_dir = []     
        default_port = 0
        if self.__install_dir:
            version_file = os.path.join(self.__install_dir, VERSION_FILE_NAME)
            if not os.path.exists(version_file):
                print("[ERROR] --path '{}' is not the installation directory " \
                      "for {}".format(self.__install_dir, self.__log_type))
                return 1
            with open(version_file, 'r') as f:
                buf = f.read()
            if self.__log_type not in buf.lower():
                print("[ERROR] --path '{}' is not the installation directory " \
                      "for {}".format(self.__install_dir, self.__log_type))
                return 1
            command = os.path.join(self.__install_dir, 'bin/sdb_sql_ctl')
            try:
                p = Popen([command, 'listinst'], stdout=PIPE)
            except OSError:
                print("[ERROR] --path '{}' is not the installation directory for " \
                      "{}".format(self.__install_dir, self.__log_type))
                return 1
            line = p.stdout.readline()
            while line:
                if os.path.isdir(line.split()[1]):
                    tmp_name = line.split()[0]
                    tmp_dir = line.split()[1]
                    if str != type(tmp_name):
                        encode_type = chardet.detect(tmp_name)
                        tmp_name = tmp_name.decode(encode_type['encoding'])
                        tmp_dir = tmp_dir.decode(encode_type['encoding'])
                    inst_name.append(tmp_name)
                    data_dir.append(tmp_dir)
                    install_dir.append(self.__install_dir)
                line = p.stdout.readline()

            if len(inst_name) == 0:
                return 0

            if self.__instance_name:
                try:
                    idx = inst_name.index(self.__instance_name)
                    inst_name = inst_name[idx:idx+1]
                    data_dir = data_dir[idx:idx+1]
                    install_dir = install_dir[idx:idx+1]
                except ValueError:
                    print("[ERROR] INSTNAME:[{}] does not " \
                          "exist".format(self.__instance_name))
                    return 1
        else:
            prefix_name = ""
            all_inst_name_in_install_dir = []
            all_data_dir_in_install_dir = []
            if self.__log_type == LOG_TYPE_MYSQL:
                prefix_name = 'sequoiasql-mysql'
                default_port = MYSQL_DEFAULT_PORT
            elif self.__log_type == LOG_TYPE_MARIADB:
                prefix_name = 'sequoiasql-mariadb'
                default_port = MARIADB_DEFAULT_PORT
            for file in os.listdir(REGISTER_CONF_PATH):
                if file.startswith(prefix_name):
                    conf = os.path.join(REGISTER_CONF_PATH, file)
                    list_conf.append(conf)

            if 0 == len(list_conf):
                default_conf = os.path.join(REGISTER_CONF_PATH, prefix_name)
                print("[ERROR] Register configuration file " \
                      "'{}' does not exist".format(default_conf))
                return 1

            for conf in list_conf:
                tmp_list1 = []
                tmp_list2 = []
                with open(conf, 'r') as f:
                    content = f.read()
                    install_conf = re.findall(r'INSTALL_DIR[^\r\n]+', content)[0]
                dir = install_conf.split("=")[1].strip()
                tmp_list1.append(dir)
                tmp_list2.append(dir)
                command = os.path.join(dir, 'bin/sdb_sql_ctl')
                p = Popen([command, 'listinst'], stdout=PIPE)
                line = p.stdout.readline()
                while line:
                    if os.path.isdir(line.split()[1]):
                        tmp_name = line.split()[0]
                        tmp_dir = line.split()[1]
                        if str != type(tmp_name):
                            encode_type = chardet.detect(tmp_name)
                            tmp_name = tmp_name.decode(encode_type['encoding'])
                            tmp_dir = tmp_dir.decode(encode_type['encoding'])
                        inst_name.append(tmp_name)
                        tmp_list1.append(tmp_name)
                        data_dir.append(tmp_dir)
                        tmp_list2.append(tmp_dir)
                        install_dir.append(dir)
                    line = p.stdout.readline()
                all_inst_name_in_install_dir.append(tmp_list1)
                all_data_dir_in_install_dir.append(tmp_list2)

            if len(inst_name) ==0:
                print("[INFO] No SQL instance found in " \
                      "{}".format(REGISTER_CONF_PATH))
                return 0

            if self.__instance_name:
                del inst_name[:]
                del data_dir[:]
                del install_dir[:]
                for i,val in enumerate(all_inst_name_in_install_dir):
                    try:
                        idx = val.index(self.__instance_name)
                        install_dir.append(val[0])
                        inst_name.append(val[idx])
                        data_dir.append(all_data_dir_in_install_dir[i][idx])
                    except ValueError:
                        continue

                if len(install_dir) == 0:
                    print("[ERROR] INSTNAME:[{}] does not " \
                          "exist".format(self.__instance_name))
                    return 1
                elif len(install_dir) > 1:
                    print("[INFO] There are {} installation directories containing " \
                          "instance '{}', which are {}. Please use --path " \
                          "to specify one of the directories you want to " \
                          "add".format(len(install_dir),
                          self.__instance_name, install_dir))
                    return 1
                    
        #read auto.cnf for port and auditpath
        for i in range(len(data_dir)):
            parser = ConfigParser.ConfigParser()
            auto_cnf = os.path.join(data_dir[i], 'auto.cnf')
            parser.read(auto_cnf)
            section = 'mysqld'
            option = 'server_audit_file_path'
            if parser.has_option(section, option):
                audit_file = parser.get(section, option)
                audit_path = os.path.dirname(audit_file)
            else:
                print("[WARNING] INSTNAME '{}' doesn't install audit " \
                      "plugin. Let's skip the instance.".format(inst_name[i]))
                continue;
            option = 'port'
            if parser.has_option(section, option):
                port = parser.get(section, option)
            else:
                port = default_port
            tmp = {}
            tmp[PORT] = port
            tmp[AUDIT_PATH] = audit_path
            tmp[ROLE] = self.__log_type
            tmp[INSTALL_DIR] = install_dir[i]
            tmp[INST_NAME] = inst_name[i]
            self.__info.append(tmp)
        return 0

    def __setup_password(self):
        file = os.path.join(MY_CONF_PATH, CONFIG_FILE_NAME)
        if not os.path.exists(file):
            print("[ERROR] Configuration file '{}' does not " \
                  "exist".format(file))
            return 1
        global_parser = ConfigParser.ConfigParser()
        global_parser.read(file)

        try:
            pwd_type = int(self.get_passwd_type(global_parser))
            if 0 != pwd_type and 1 != pwd_type:
                print("[ERROR] 'w_type' in configuration file '{}' " \
                      "is invalid".format(file))
                return 1
            if 0 == pwd_type:
                passwd = CryptoUtil.encrypt(self.get_passwd(global_parser))
                global_parser.set(KW_MONITOR, KW_PASSWD, passwd)
                global_parser.set(KW_MONITOR, KW_PASSWD_TYPE, 1)
                global_parser.write(open(file, 'w'))
        except ValueError:
            print("[ERROR] 'w_type' in configuration file '{}' " \
                  "must be integer".format(file))
            return 1
        except ConfigParser.NoOptionError:
            pass
            
        return 0

    def __add_obj(self):
        audit_path = []
        if self.__log_type == LOG_TYPE_SDB:
            rc = self.__parse_sdb_info()
        else:
            rc = self.__parse_sql_inst_info()
        if 0 != rc:
            return rc
        return self.__create_work_dir()
  
    def __filter_obj(self, file):
        with open(file, 'r') as f:
            buf = f.read()
        if self.__install_dir:
            install_conf = re.findall(r'installdir[^\r\n]+', buf)[0]
            install_dir= install_conf.split('=')[1].strip()
            if self.__install_dir != install_dir:
                return
            if self.__log_type == LOG_TYPE_MYSQL or self.__log_type == LOG_TYPE_MARIADB:
                inst_name_conf = re.findall(r'instname[^\r\n]+', buf)[0]
                inst_name = inst_name_conf.split('=')[1].strip()
                if self.__instance_name and self.__instance_name != inst_name:
                    return False
                else:
                    return True
            else:
                return True
        else:
            if self.__log_type == LOG_TYPE_MYSQL or self.__log_type == LOG_TYPE_MARIADB:
                inst_name_conf = re.findall(r'instname[^\r\n]+', buf)[0]
                inst_name = inst_name_conf.split('=')[1].strip()
                if self.__instance_name and self.__instance_name != inst_name:
                    return False
                else:
                    return True
            else:
                return True

    def __del_obj(self):
        del_dir = os.path.join(MY_CONF_PATH, self.__log_type)
        if not os.path.exists(del_dir):
            print("[INFO] Audit object({}) doesn't exist".format(self.__log_type))
            return 0

        has_matched = False
        for file in os.listdir(del_dir):
            cur_path = os.path.join(del_dir, file)
            if not os.path.isdir(cur_path):
                continue
            cur_config_file = os.path.join(cur_path, CONFIG_FILE_NAME)
            if not os.path.exists(cur_config_file):
                continue
            matched = self.__filter_obj(cur_config_file)
            if not matched:
                continue
            has_matched = True
            pid_file = os.path.join(cur_path, PID_FILE_NAME)
            if os.path.exists(pid_file):
                with open(pid_file, 'r') as f:
                    pid = int(f.readline())
                if pid_exist(pid):
                    os.kill(pid, 15)
                    for i in range(PROCESS_WAIT_TIME):
                        if not pid_exist(pid):
                            break
                        time.sleep(0.1)
                    if pid_exist(pid):
                        print("Deleted node({}) of {} " \
                              "failed".format(file, self.__log_type))
                        continue
            shutil.rmtree(cur_path)
            print("Deleted node({}) of {} successfully".format(file, self.__log_type))

        if not has_matched:
            if self.__instance_name:
                print("[INFO] Instance '{}' doesn't exist".format(self.__instance_name))
            elif self.__install_dir:
                print("[INFO] No nodes found in the path '{}'".format(self.__install_dir))
            else:
                print("[INFO] No nodes found")
            return 0

        if 0 == len(os.listdir(del_dir)):
            shutil.rmtree(del_dir)
        return 0

    def __list_obj(self):
        node_num = 0
        running_node_num = 0
        all_role = []
        all_pid = []
        all_svc_name = []
        all_audit_path = []
        all_log_path = []
        for obj_dir in os.listdir(MY_CONF_PATH):
            obj_path = os.path.join(MY_CONF_PATH, obj_dir)
            if (not os.path.isdir(obj_path)) or (obj_dir != LOG_TYPE_SDB \
                                             and obj_dir != LOG_TYPE_MYSQL \
                                             and obj_dir != LOG_TYPE_MARIADB):
                continue
            for node_dir in os.listdir(obj_path):
                is_alive_pid = '-'
                node_path = os.path.join(obj_path, node_dir)
                if not os.path.isdir(node_path):
                    continue
                cur_config_file = os.path.join(node_path, CONFIG_FILE_NAME)
                if not os.path.exists(cur_config_file):
                    continue
                node_num+=1
                local_parser = ConfigParser.ConfigParser()
                local_parser.read(cur_config_file)
                pid_file = os.path.join(node_path, PID_FILE_NAME)
                if os.path.exists(pid_file):
                    with open(pid_file, 'r') as f:
                        pid = str(f.readline())
                    if pid_exist(int(pid)):
                        is_alive_pid = str(pid)
                        running_node_num+=1
                    else:
                        is_alive_pid = '-'
                all_role.append(self.get_role(local_parser))
                all_pid.append(is_alive_pid)
                all_svc_name.append(self.get_port(local_parser))
                all_audit_path.append(self.get_audit_path(local_parser))
                all_log_path.append(node_path)
        if 0 != len(all_role):
            out_role_len = max(len(s) for s in all_role) + FORMAT_NUM
            out_pid_len = max(len(s) for s in all_pid) + FORMAT_NUM
            out_svc_name_len = max(len(s) for s in all_svc_name) + FORMAT_NUM
            out_audit_path_len = max(len(s) for s in all_audit_path) + FORMAT_NUM
            out_log_path_len = max(len(s) for s in all_log_path) + FORMAT_NUM
        else:
            out_role_len = len("ROLE") + FORMAT_NUM
            out_pid_len = len("PID") + FORMAT_NUM
            out_svc_name_len = len("SVCNAME") + FORMAT_NUM
            out_audit_path_len = len("AUDITPATH") + FORMAT_NUM
            out_log_path_len = len("LOGPATH") + FORMAT_NUM
        print("%-{}s%-{}s%-{}s%-{}s%-{}s".format(out_role_len, out_pid_len,
              out_svc_name_len, out_audit_path_len, out_log_path_len) % 
              ("ROLE", "PID", "SVCNAME", "AUDITPATH", "LOGPATH"))
        for i in range(len(all_role)):
            print("%-{}s%-{}s%-{}s%-{}s%-{}s".format(out_role_len, out_pid_len,
                  out_svc_name_len, out_audit_path_len, out_log_path_len) % 
                  (all_role[i], all_pid[i], all_svc_name[i], all_audit_path[i],
                  all_log_path[i]))
        print("Total: {}; Run: {}".format(node_num, running_node_num))
        return 0
        
    def __start_obj(self):
        rc = self.__setup_password()
        if 0 != rc:
            return rc

        node_num = 0
        pid = 0
        all_port = []
        exist_pid = []
        all_exec_para = []
        all_node_path = []
        try:
            for obj_dir in os.listdir(MY_CONF_PATH):
              obj_path = os.path.join(MY_CONF_PATH, obj_dir)
              if os.path.isdir(obj_path) and (obj_dir == LOG_TYPE_SDB \
                                         or obj_dir == LOG_TYPE_MYSQL \
                                         or obj_dir == LOG_TYPE_MARIADB):
                  self.set_log_type(obj_dir)
                  for node_dir in os.listdir(obj_path):
                      node_path = os.path.join(obj_path, node_dir)
                      if os.path.isdir(node_path):
                          parameters = '-t {} '.format(self.get_log_type())
                          cur_config_file = os.path.join(node_path, CONFIG_FILE_NAME) 
                          parameters += '-c {}'.format(cur_config_file)
                          all_exec_para.append(parameters)
                          all_node_path.append(node_path)
                          all_port.append(node_dir)
                          os.chdir(node_path)
                          pid_file = os.path.join(node_path, PID_FILE_NAME)
                          skip = False
                          if os.path.exists(pid_file):
                              with open(pid_file, 'r') as f:
                                  pid = int(f.readline())
                              if pid_exist(pid):
                                  skip = True
                          if not skip:
                              log_file = os.path.join(node_path, LOG_FILE_NAME)
                              os.system("nohup python {} {} >> {} " \
                                        "2>&1 &".format(EXPORTER_EXEC,
                                        parameters, log_file))
                          else:
                              print("Exporter({}) already " \
                                    "exists(PID: {})".format(node_dir, pid))
                              exist_pid.append(pid)
                          node_num += 1
            
        except Exception as error:
            print("[ERROR] Run exporter failed: " + str(error))
            raise

        if node_num == 0:
            print("No node exists")
            return 0

        # all nodes has started
        if len(exist_pid) == len(all_node_path):
            return 0

        # timed check
        has_sucess = False
        time.sleep(PROCESS_WAIT_TIME)
        for i,node_path in enumerate(all_node_path):
            pid_file = os.path.join(node_path, PID_FILE_NAME)
            if os.path.exists(pid_file):
                with open(pid_file, 'r') as f:
                    pid = int(f.readline())
                if not pid_exist(pid):
                    print("[ERROR] Failed to start exporter({}). Please " \
                          "verify log '{}'".format(all_port[i],
                          os.path.join(node_path, LOG_FILE_NAME)))
                else:
                    if pid in exist_pid:
                        continue
                    has_sucess = True
                    print("Starting exporter({}) " \
                          "success (PID: {})".format(all_port[i], pid))
            else:
                print("[ERROR] Failed to start exporter({}). Please " \
                      "verify log '{}'".format(all_port[i],
                      os.path.join(node_path, LOG_FILE_NAME)))

        # start sdbaudit_daemon
        if not has_sucess:
            return 0
        daemon_exec = os.path.join(MY_HOME, "sdbaudit_daemon.py")
        daemon_pid_file = os.path.join(MY_HOME, DAEMON_PID_FILE_NAME)
        if not os.path.exists(daemon_pid_file):
            os.system("nohup python {} --start > /dev/null " \
                      "2>&1 &".format(daemon_exec))
        else:
            with open(daemon_pid_file, 'r') as f:
                pid = int(f.readline())
            if not pid_exist(pid):
                os.system("nohup python {} --start > /dev/null " \
                          "2>&1 &".format(daemon_exec))
        return 0
                        
    def __stop_obj(self):
        stopped_num = 0
        all_pid = []
        all_port = []
        all_pid_file = []
        for obj_dir in os.listdir(MY_CONF_PATH):
            obj_path = os.path.join(MY_CONF_PATH, obj_dir)
            if (not os.path.isdir(obj_path)) or (obj_dir != LOG_TYPE_SDB \
                                             and obj_dir != LOG_TYPE_MYSQL \
                                             and obj_dir != LOG_TYPE_MARIADB):
                continue
            for node_dir in os.listdir(obj_path):
                node_path = os.path.join(obj_path, node_dir)
                if not os.path.isdir(node_path):
                    continue
                pid_file = os.path.join(node_path, PID_FILE_NAME)
                if not os.path.exists(pid_file):
                    continue
                with open(pid_file, 'r') as f:
                    pid = int(f.readline())
                all_pid_file.append(pid_file)
                all_port.append(node_dir)
                all_pid.append(pid)
                if pid_exist(pid):
                    os.kill(pid, 15)
                    stopped_num += 1

        if stopped_num == 0:
            print("No running nodes")

        for i in range(PROCESS_WAIT_TIME):
            remain_pid = []
            remain_port = []
            has_alive = False
            for i,pid in enumerate(all_pid):
                if pid_exist(pid):
                    has_alive = True
                    remain_pid.append(pid)
                    remain_port.append(all_port[i])
                else:
                    print("Stopping exporter({}) success " \
                          "(PID: {})".format(all_port[i], pid))
            if not has_alive:
                return 0
            else:
                all_pid = remain_pid
                all_port = remain_port
                time.sleep(1)
        for i,port in enumerate(all_port):
            print("[ERROR] Failed to stop exporter({}), ".format(port))
        return 1
            
    def run_command(self):
        if self.__operate == 'add':
            rc = self.__add_obj()
        elif self.__operate == 'del':
            rc = self.__del_obj()
        elif self.__operate == 'list':
            rc = self.__list_obj()
        elif self.__operate == 'start':
            rc = self.__start_obj()
        elif self.__operate == 'stop':
            rc = self.__stop_obj()
        return rc

    def get_option(self, parser, section, option):
        return parser.get(section, option)

    def set_log_type(self, log_type):
        self.__log_type = log_type

    def get_log_type(self):
        return self.__log_type

    def get_passwd(self, parser):
        return self.get_option(parser, KW_MONITOR, KW_PASSWD)

    def get_passwd_type(self, parser):
        return self.get_option(parser, KW_MONITOR, KW_PASSWD_TYPE)

    def get_use_ssl(self, parser):
        return self.get_option(parser, KW_MONITOR, KW_SSL)

    def get_is_del_file(self, parser):
        return self.get_option(parser, KW_MONITOR, KW_DELETE)

    def get_insert_num(self, parser):
        return self.get_option(parser, KW_MONITOR, KW_INSERT_NUM)

    def get_audit_path(self, parser):
        return self.get_option(parser, KW_MONITOR, KW_AUDIT_PATH)

    def get_role(self, parser):
        return self.get_option(parser, KW_MONITOR, KW_ROLE)

    def get_node_name(self, parser):
        return self.get_option(parser, KW_MONITOR, KW_NODE_NAME)

    def get_port(self, parser):
        return self.get_node_name(parser).split(':')[1]

def main():
    #Setup the command parser
    program = os.path.basename(__file__.replace(".py", ""))
    optMgr = OptionsMgr(program)
    rc = optMgr.parse_operation()
    if 0 != rc: 
      return rc

    #Object management
    objMgr = ObjMgr(optMgr)
    return objMgr.run_command()

if __name__ == '__main__':
    rc = main()
    sys.exit(rc)

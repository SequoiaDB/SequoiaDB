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

import os
import sys
import re
import time
import base64
import random
import signal
import socket
import datetime
import operator
import optparse
import logging.config
import pysequoiadb
try:
    import ConfigParser as ConfigParser
except Exception:
    import configparser as ConfigParser
from getpass import getpass
from pysequoiadb import client
from collections import OrderedDict
from pysequoiadb.collection import INSERT_FLG_CONTONDUP
from pysequoiadb.errcode import *
from pysequoiadb.error import (SDBTypeError, SDBBaseError)

try:
    reload(sys)
    sys.setdefaultencoding('utf8')
except NameError:
    import importlib,sys
    importlib.reload(sys)

DESCRIPTION = '''%prog is a sdbaudit exporter server.'''

USAGE = "%prog -t <sdb|mysql|mariadb> --auditpath[=]AUDITPATH " \
        "--clname[=]CS.CL [OPTION]..."

PROCESS_WAIT_TIME = 3
MAX_RETRY_TIMES = 3

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
KW_PASSWD_TYPE = 'password_type'

FIELD_ID = 'ID'
FIELD_PID = 'PID'
FIELD_TID = 'TID'
FIELD_ROLE = 'Role'
FIELD_TYPE = 'Type'
FIELD_FROM = 'From'
FIELD_ACTION = 'Action'
FIELD_RESULT = 'Result'
FIELD_CLIENT = 'Client'
FIELD_MESSAGE = 'Message'
FIELD_SVC_NAME = 'SvcName'
FIELD_USER_NAME = 'UserName'
FIELD_HOST_NAME = 'HostName'
FIELD_TIMESTAMP = 'Timestamp'
FIELD_OBJECT_TYPE = 'ObjectType'
FIELD_OBJECT_NAME = 'ObjectName'
FIELD_CONNECT_ID = 'ConnectionID'
FIELD_OPERATION_ID = 'OperationID'

LOG_TYPE_MYSQL = 'mysql'
LOG_TYPE_SDB = 'sdb'
LOG_TYPE_MARIADB = 'mariadb'
PID_FILE_NAME='sdbaudit.pid'
CONFIG_FILE_NAME = "sdbaudit.conf"
LOG_CONF_FILE_NAME='sdbaudit_log.conf'
STATUS_FILE_NAME = 'sdbaudit.status'

MY_HOME = os.path.abspath(os.path.dirname(__file__))
MY_CONF_PATH = os.path.join(MY_HOME, 'conf')


def pid_exist(pid):
    try:
        os.kill(pid, 0)
    except OSError:
        return False
    return True

def string_to_bool(str):
    if str.lower() == 'true':
        return True
    else:
        return False

class CryptoUtil:
    def __init__(self):
        pass

    @classmethod
    def encrypt(cls, source_str):
        random_choice = ("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
                         "1234567890!@#$%^&*()")
        to_encrypt_arr = []
        shift_str = ""
        for char in source_str:
            shift_str = shift_str + chr(ord(char) + 3)
        shift_index = 0
        for index in range(0, len(shift_str) * 3):
            if index % 3 != 0:
                rand_char = random.choice(random_choice)
                to_encrypt_arr.append(rand_char)
            else:
                to_encrypt_arr.append(shift_str[shift_index])
                shift_index = shift_index + 1
        to_encrypt_str = ''.join(to_encrypt_arr)
        encrypt_str = base64.b64encode(to_encrypt_str)
        return encrypt_str

    @classmethod
    def decrypt(cls, encrypt_str):
        decrypt_str = base64.b64decode(encrypt_str)
        shift_str = []
        for index in range(len(decrypt_str)):
            if index % 3 == 0:
                shift_str.append(decrypt_str[index])
        source_arr = []
        for char in shift_str:
            source_arr.append(chr(ord(char) - 3))
        source_str = "".join(source_arr)
        return source_str

class Logger:
    def __init__(self):
        self.logger = None

    def init(self, log_config_file):
        # Get the log file path from the log configuration file, and create
        # the directory if it dose not exist.
        config_parser = ConfigParser.ConfigParser()
        files = config_parser.read(log_config_file)
        if len(files) != 1:
            print("[ERROR] Read log configuration file '{}' failed".format(log_config_file))
            return 1
        logging.config.fileConfig(log_config_file)
        self.logger = logging.getLogger("logExporter")
        return 0

    def get_logger(self):
        return self.logger

class OptionArgs():
    def __init__(self, options):
        self.__options = options

    def get_log_type(self):
        return self.__options.log_type

    def get_audit_path(self):
        return self.__options.audit_path

    def get_is_del_file(self):
        return self.__options.delete

    def get_host(self):
        return self.__options.addr.split(":")[0]

    def get_port(self):
        return self.__options.addr.split(":")[1]

    def get_user_name(self):
        return self.__options.user_name

    def get_passwd(self):
        return self.__options.passwd

    #def get_cipher(self):
    #    return string_to_bool(self.__options.cipher)

    #def get_token(self):
    #    return self.__options.token

    #def get_cipher_file(self):
    #    return self.__options.cipher_file

    def get_use_ssl(self):
        return self.__options.use_ssl

    def get_cl_full_name(self):
        return self.__options.cl_name
    
    def get_cs_name(self):
        return self.__options.cl_name.split(".")[0]

    def get_cl_name(self):
        return self.__options.cl_name.split(".")[1]

    def get_insert_num(self):
        return self.__options.insert_num

    def get_role(self):
        return self.__options.role

    def get_node_name(self):
        return self.__options.node_name

    def get_audit_log_name(self):
        if self.get_log_type() == LOG_TYPE_SDB:
            return "sdbaudit.log"
        else:
            return "server_audit.log"

class OptionsMgr:
    def __init__(self, program):
        self.__parser = optparse.OptionParser(
        description = DESCRIPTION,
        usage = USAGE,
        prog = program
        )
        self.__options = {
            KW_MONITOR: {
                KW_USER : '',
                KW_PASSWD : '',
                KW_PASSWD_TYPE : 0,
                KW_ADDR : 'localhost:11810',
                #KW_CIPHER : 'false',
                #KW_TOKEN : '',
                #KW_CIPHERFILE : './passwd',
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
        self.__global_parser = None
        self.args = None

    def __add_option(self):
        #Add -t option
        self.__parser.add_option("-t", "--type", action='store', type="string",
                                 dest='log_type', help="audit log type. " \
                                 "Option: sdb, mysql, mariadb")
        #Add -c option
        self.__parser.add_option("-c", "--conf", action='store', type="string",
                                 dest='conf_path', help="config file path")
        #Add --path option
        self.__parser.add_option("--auditpath", action='store', type="string",
                                 dest='audit_path', help="audit log file " \
                                 "paths, which must be a directory")
        #Add --delete option
        self.__parser.add_option("--delete", action='store', type='string',
                                 dest="delete", help="whether to remove the " \
                                 "audit log file that has finished " \
                                 "exporting, only the full file is deleted, " \
                                 "(arg: [true|false])")
        #Add --addr option
        self.__parser.add_option("--addr", action='store', type='string',
                                 dest="addr", help="host " \
                                 "addressed(hostname:svcname), default: " \
                                 "'localhost:11810'")
        #Add --user option
        self.__parser.add_option("-u", "--user", action='store', type='string',
                                 dest="user_name", help="username")
        #Add --password option
        self.__parser.add_option("-w", "--password", action='store',
                                 type='string', dest="passwd", help="password")
        #Add --password_type option
        self.__parser.add_option("--w_type", action='store', type='int',
                                 dest="passwd_type", help="password type. " \
                                 "'0' for plaintext, '1' for ciphertext")
        #Add --cipher option
        #self.__parser.add_option("--cipher", action='store', type='string',
        #                         dest="cipher", help="input password using a " \
        #                        "cipher file, (arg: [true|false]")
        #add --token option
        #self.__parser.add_option("--token", action='store', type='string',
        #                         dest="token", help="password encryption token")
        #Add --cipherfile option
        #self.__parser.add_option("--cipherfile", action='store', type='string',
        #                         dest="cipher_file", help="input password " \
        #                         "using a cipher file")
        #Add --ssl option
        self.__parser.add_option("--ssl", action='store', type='string',
                                 dest="use_ssl", help="set SSL connection " \
                                 "(arg: [true|false])")
        #Add --clname option
        self.__parser.add_option("--clname", action='store', type='string',
                                 dest="cl_name", help="collection full name")
        #Add --insertnum option
        self.__parser.add_option("--insertnum", action='store', type='int',
                                 dest="insert_num", help="maximum number of " \
                                 "records per bulk insert")
        #Add --role option
        self.__parser.add_option("--role", action='store', type='string',
                                 dest="role", help="node role, sdb:[coord, " \
                                 "catalog, data, om, standalone], mysql, mariadb")
        #Add --nodename option
        self.__parser.add_option("--nodename", action='store', type='string',
                                 dest="node_name", help="node name, " \
                                 "<hostname>:<svcname>")

    def __load_global_configs(self):
        file = os.path.join(MY_CONF_PATH, CONFIG_FILE_NAME)
        if not os.path.exists(file):
            logger.error("Configuration file {} does not " \
                  "exist".format(CONFIG_FILE_NAME))
            return 1
        self.__global_parser = ConfigParser.ConfigParser()
        self.__global_parser.read(file)
        for section, options in self.__options.iteritems():
            for option, value in options.iteritems():
                if not self.__global_parser.has_option(section, option):
                    if not self.__global_parser.has_section(section):
                        self.__global_parser.add_section(section)
                    self.__global_parser.set(section, option, str(value))
        return 0

    def __validate_and_format_option(self, options):
        # --type
        if not options.log_type:
            logger.error("Log type can not be null. Please use -t to specify.")
            return 1

        log_type = options.log_type.lower()
        if log_type != LOG_TYPE_SDB and log_type != LOG_TYPE_MYSQL and log_type != LOG_TYPE_MARIADB:
            logger.error("Invalid log type. Please use 'sdb','mysql' or " \
                  "'mariadb' instead.")
            return 1
        options.log_type = log_type

        # --auditpath
        if len(options.audit_path) == 0:
            logger.error("Audit path can not be null. Please use --auditpath to " \
                  "specify.")
            return 1
        elif not os.path.exists(options.audit_path):
            logger.error("Directory of audit path '{}' does not exist".format(options.audit_path))
            return 1
        elif not os.path.isdir(options.audit_path):
            logger.error("Audit path '{}' must be a directory".format(options.audit_path))
            return 1

        # --delete
        need_delete = options.delete.lower()
        if need_delete != "true" and need_delete != "false":
            logger.error("--delete option '{}' is not a boolean(true/false)."
                         .format(options.delete))
            return 1
        options.delete = string_to_bool(need_delete)

        # --addr
        if options.addr.find(':') == -1:
            logger.error("Wrong format of address. No svcname found.")
            return 1

        # --user --password have nothing to check

        # --w_type
        is_w_type_valid = False
        if isinstance(options.passwd_type, str):
            if options.passwd_type == '0' or options.passwd_type == '1':
                options.passwd_type = int(options.passwd_type)
                is_w_type_valid = True
        elif isinstance(options.passwd_type, int):
            if options.passwd_type == 0 or options.passwd_type == 1:
                is_w_type_valid = True
        if not is_w_type_valid:
            logger.error("Password type '{}' is invalid.".format(options.passwd_type))
            return 1

        # --ssl
        use_ssl = options.use_ssl.lower()
        if use_ssl != "true" and use_ssl != "false":
            logger.error("--ssl option '{}' is not a boolean(true/false)."
                         .format(options.use_ssl))
            return 1
        options.use_ssl = string_to_bool(use_ssl)

        # --clname
        if options.cl_name.find('.') == -1:
            logger.error("--clname option '{}' is not a collection full name."
                         .format(options.cl_name))
            return 1

        # --insertnum
        if isinstance(options.insert_num, str):
            try:
                options.insert_num = int(options.insert_num)
            except (ValueError):
                logger.error("--insertnum option '{}' is not an integer."
                             .format(options.insert_num))
                return 1
        if isinstance(options.insert_num, int):
            if options.insert_num < 1:
                logger.error("--insertnum option '{}' must be greater than 0."
                             .format(options.insert_num))
                return 1

        # --role has nothing to check

        # --nodename
        if options.node_name.find(':') == -1:
            logger.error("--nodename is with wrong format. No svcname found.")
            return 1

        return 0

    def parse_option(self):
        self.__add_option()
        options,args = self.__parser.parse_args()
        if options.conf_path:
            if not os.path.isfile(options.conf_path):
                logger.error("configuration path '{}' is not a file".format(options.conf_path))
                return 1
            rc = self.__load_global_configs()
            if 0 != rc:
                return rc
            local_parser = ConfigParser.ConfigParser()
            local_parser.read(options.conf_path)
            for section in self.__global_parser.sections():
                for option in self.__global_parser.options(section):
                    if not local_parser.has_option(section, option):
                        if not local_parser.has_section(section):
                            local_parser.add_section(section)
                        local_parser.set(section, option, 
                                        self.__global_parser.get(section, option))
            if not options.audit_path:
                options.audit_path = local_parser.get(KW_MONITOR, KW_AUDIT_PATH)
            if not options.delete:
                options.delete = local_parser.get(KW_MONITOR, KW_DELETE)
            if not options.addr:
                options.addr = local_parser.get(KW_MONITOR, KW_ADDR)
            if not options.user_name:
                options.user_name = local_parser.get(KW_MONITOR, KW_USER)
            if not options.passwd:
                options.passwd = local_parser.get(KW_MONITOR, KW_PASSWD) 
            if not options.passwd_type:
                options.passwd_type = local_parser.get(KW_MONITOR,
                                                           KW_PASSWD_TYPE)
#            if not options.cipher:
#                options.cipher = local_parser.get(KW_MONITOR, KW_CIPHER)
#            if not options.token:
#                options.token = local_parser.get(KW_MONITOR, KW_TOKEN)
#            if not options.cipher_file:
#                options.cipher_file = local_parser.get(KW_MONITOR,
#                                                       KW_CIPHERFILE)
            if not options.use_ssl:
                options.use_ssl = local_parser.get(KW_MONITOR, KW_SSL)
            if not options.cl_name:
                options.cl_name = local_parser.get(KW_MONITOR, KW_CL_NAME)
            if not options.insert_num:
                options.insert_num = local_parser.get(KW_MONITOR, KW_INSERT_NUM)
            if not options.role:
                options.role = local_parser.get(KW_MONITOR, KW_ROLE)
            if not options.node_name:
                options.node_name = local_parser.get(KW_MONITOR, KW_NODE_NAME)

        rc = self.__validate_and_format_option(options)
        if 0 != rc:
            return rc

        if options.passwd_type == 1:
            options.passwd = CryptoUtil.decrypt(options.passwd)
        elif options.passwd_type == 0:
            passwd = CryptoUtil.encrypt(options.passwd)
            new_local_parser = ConfigParser.ConfigParser()
            new_local_parser.read(options.conf_path)
            new_local_parser.set(KW_MONITOR, KW_PASSWD, passwd)
            new_local_parser.set(KW_MONITOR, KW_PASSWD_TYPE, '1')
            new_local_parser.write(open(options.conf_path, 'w'))

        self.args = OptionArgs(options)
        return 0

class StatMgr:
    """ Status manager is responsible for loading, reading and updating the
        status file of the reporter work
    """
    def __init__(self, stat_file):
        self.parser = None
        self.stat_file = stat_file
        self.file_inode = 0
        self.last_parsed_row = 0

    def __init_stat_file(self):
        self.file_inode = 0
        self.set_last_parsed_row(0)
        self.update_stat()

    def load_stat(self):
        self.parser = ConfigParser.ConfigParser()
        try:
            if not os.path.exists(self.stat_file):
                # No status file at all. Treat as fresh start.
                logger.warn('Status file {} does not exist. Init it with '
                            'default values'.format(self.stat_file))
                self.__init_stat_file()

            self.parser.read(self.stat_file)
            stat_sec_name = 'status'
            self.file_inode = int(self.parser.get(stat_sec_name, 'file_inode'))
            self.last_parsed_row = int(self.parser.get(stat_sec_name,
                                                      'last_parsed_row'))
        except Exception as error:
            logger.error('Load status failed: ' + str(error))
            return 1

    def get_file_inode(self):
        return self.file_inode

    def set_file_inode(self, file_inode):
        self.file_inode = file_inode

    def get_last_parsed_row(self):
        return self.last_parsed_row

    def set_last_parsed_row(self, row):
        self.last_parsed_row = row

    def update_stat(self):
        stat_sec_name = 'status'
        if not self.parser.has_section(stat_sec_name):
            self.parser.add_section(stat_sec_name)

        self.parser.set(stat_sec_name, 'file_inode', self.file_inode)
        self.parser.set(stat_sec_name, "last_parsed_row", self.last_parsed_row)
        self.parser.write(open(self.stat_file, 'w'))

class SdbConnect:
    
    def __init__(self, args):
        self.args = args
        self.__host = self.args.get_host()
        self.__port = self.args.get_port()
        self.__user = self.args.get_user_name()
        self.__passwd = self.args.get_passwd()
        self.__cs_name = self.args.get_cs_name()
        self.__cl_name = self.args.get_cl_name()
        self.__use_ssl = self.args.get_use_ssl()
        self.__has_connect = False
        self.__connection = None
        self.__cl = None

    def ensure_cl(self):
        index_name = 'UniqueID'
        index = OrderedDict([('ID', 1), ('NodeName', 1)])
        self.__connection = client(self.__host, self.__port, self.__user,
                                   self.__passwd, self.__use_ssl)
        self.__has_connect = True

        # ensure cs
        try:
            cs = self.__connection.get_collection_space(self.__cs_name)
        except (SDBTypeError, SDBBaseError) as e:
            if SDB_DMS_CS_NOTEXIST != get_errcode(e.code):
                raise e
            try:
                self.__connection.create_collection_space(self.__cs_name)
            except (SDBTypeError, SDBBaseError) as err:
                if SDB_DMS_CS_EXIST != get_errcode(err.code):
                    raise err
            cs = self.__connection.get_collection_space(self.__cs_name)

        # ensure cl
        try:
            self.__cl = cs.get_collection(self.__cl_name)
        except (SDBTypeError, SDBBaseError) as e:
            if SDB_DMS_NOTEXIST != get_errcode(e.code):
                raise e
            try:
                cs.create_collection(self.__cl_name)
            except (SDBTypeError, SDBBaseError) as err:
                if SDB_DMS_EXIST != get_errcode(err.code):
                    raise err
            self.__cl = cs.get_collection(self.__cl_name)

        # ensure index
        if not self.__cl.is_index_exist(index_name):
            try:
                self.__cl.create_index(index, index_name, True)
            except (SDBTypeError, SDBBaseError) as e:
                if SDB_IXM_REDEF != get_errcode(e.code):
                    raise e

    def write_row(self, records):
        retry_times = MAX_RETRY_TIMES
        while retry_times >= 0 :
            try:
                self.__cl.bulk_insert(INSERT_FLG_CONTONDUP, records)
                break
            except (SDBTypeError, SDBBaseError) as e:
                if (not self.__is_net_error(e)) or retry_times == 0:
                    logger.info("Failed to insert logs, exception: {}".format(e))
                    raise e

            time.sleep(PROCESS_WAIT_TIME)
            logger.info("retry to insert logs for exception: {}".format(e))
            try:
                if not self.__connection.is_valid():
                    self.__connection.disconnect()
                    self.__connection = client(self.__host, self.__port, self.__user,
                                               self.__passwd, self.__use_ssl)
                    cs = self.__connection.get_collection_space(self.__cs_name)
                    self.__cl = cs.get_collection(self.__cl_name)
            except (SDBTypeError, SDBBaseError) as e:
                if not self.__is_net_error(e):
                    raise e

            retry_times -= 1

    def __is_net_error(self, e):
        return SDB_NETWORK_CLOSE == get_errcode(e.code) or \
               SDB_NETWORK == get_errcode(e.code) or \
               SDB_NOT_CONNECTED == get_errcode(e.code) or \
               SDB_NET_CANNOT_CONNECT == get_errcode(e.code)

    def __del__(self):
        if self.__has_connect:
            self.__connection.disconnect()
            self.__has_connect = False

class LogExporter:
    """ parsing audit log and reporter into sdb collection """

    def __init__(self, args, stat_mgr, log_type, connect):
        self.stat_mgr = stat_mgr
        self.connect = connect
        self.__log_type = log_type
        self.__audit_path = args.get_audit_path()
        self.__audit_log_name = args.get_audit_log_name()
        self.__role = args.get_role()
        self.__node_name = args.get_node_name()
        self.__insert_num = args.get_insert_num()
        self.__cl_full_name = args.get_cl_full_name()
        self.__delete_file = args.get_is_del_file()
        self.__records = []
        self.__num_of_records = 0
        self.__buf = ""

    def __is_valid_suffix(self, suffix):
        if self.__log_type == LOG_TYPE_SDB:
            try:
                time.strptime(suffix, "%Y-%m-%d-%H:%M:%S")
                return True
            except ValueError:
                return False
        else:
            return suffix.isdigit()

    def __get_audit_file_list(self, reverse_order):
        audit_list = []
        audit_path = self.__audit_path
        audit_file_name = self.__audit_log_name

        file_list = os.listdir(audit_path)
        if 0 == len(file_list):
            return audit_list

        file_list = sorted(file_list, reverse=reverse_order)

        for f in file_list:
            if f.startswith(audit_file_name):
                if len(f) > len(audit_file_name):
                    suffix = os.path.splitext(f)[-1]
                    suffix_num = suffix[1:]
                    if not self.__is_valid_suffix(suffix_num):
                        continue
                    else:
                        audit_list.append(f)
                else:
                    audit_list.append(f)

        if self.__log_type == LOG_TYPE_SDB:
            # new_audit_log ----> old_audit_log
            if reverse_order:
                try:
                    audit_list.remove(audit_file_name)
                    audit_list.insert(0, audit_file_name)
                except ValueError:
                    pass
            # old_audit_log ----> new_audit_log
            else:
                try:
                    audit_list.remove(audit_file_name)
                    audit_list.append(audit_file_name)
                except ValueError:
                    pass
        return audit_list

    def __check_audit_file_exist(self, audit_path):
        audit_file_list = self.__get_audit_file_list(False)
        if len(audit_file_list) == 0:
            logging.warn("No audit file in the audit path " \
                         "{}".format(audit_path))

    def __get_eldest_audit_file(self):
        audit_path = self.__audit_path
        audit_file_name = self.__audit_log_name
        audit_file = os.path.join(audit_path, audit_file_name)
        if self.__log_type == LOG_TYPE_SDB:
            reverse_order = False  
        else:
            reverse_order = True

        self.__check_audit_file_exist(audit_path)
        while True:
            audit_file_list = self.__get_audit_file_list(reverse_order)
            if len(audit_file_list) == 0:
                time.sleep(PROCESS_WAIT_TIME)
                continue
            audit_inode_before = os.stat(audit_file).st_ino
            for f in audit_file_list:
                file_path = os.path.join(audit_path, f)
                cur_inode = os.stat(file_path).st_ino
                try:
                    fd = open(file_path, 'r')
                    audit_inode_after = os.stat(audit_file).st_ino
                    if audit_inode_after != audit_inode_before:
                        fd.close()
                        break
                    else:
                        return cur_inode, fd
                except IOError:
                    break

    def __get_next_file(self):
        if 0 == self.stat_mgr.get_file_inode():
            return self.__get_eldest_audit_file()
        audit_path = self.__audit_path
        base_file = os.path.join(audit_path, self.__audit_log_name)
        if self.__log_type == LOG_TYPE_SDB:
            reverse_order = True  
        else:
            reverse_order = False

        while True:
            found_file = False
            pre_file_inode = 0
            base_inode_before = os.stat(base_file).st_ino
            audit_file_list = self.__get_audit_file_list(reverse_order)
            if len(audit_file_list) == 0:
                logging.info("No audit file in the audit " \
                             "path {}".format(audit_path))
                time.sleep(PROCESS_WAIT_TIME)
                continue
            for file in audit_file_list:
                file_path = os.path.join(audit_path, file)
                cur_file_stat = os.stat(file_path)
                cur_file_inode = cur_file_stat.st_ino
                if 0 == cur_file_stat.st_size:
                    if 0 == pre_file_inode:
                        return cur_file_inode, None
                    else:
                        logging.error('Audit file {} with inode {} is '
                                      'empty'.format(file, cur_file_inode))
                        return 0, None

                if cur_file_inode == self.stat_mgr.get_file_inode():
                    # Found the file with the expected inode id. Check if
                    # all records in the file have been processed.
                    found_file = True
                    base_inode_after = os.stat(base_file).st_ino
                    try:
                        fd = open(file_path, 'r')
                        if base_inode_after != base_inode_before:
                            # File list changed. Need to check again.
                            fd.close()
                            break
                    except IOError:
                        # If we can not open the file, the target file is
                        #really gone, and incremental synchronization is
                        #not possible any longer.
                        logging.error(
                            'Audit file {} not found'.format(file))
                        return 0, None
                    index = -1
                    for index, line in enumerate(fd):
                         pass
                    fd.seek(0)
                    line_num = index + 1
                    last_parsed_row = self.stat_mgr.get_last_parsed_row()
                    if line_num > last_parsed_row:
                        return cur_file_inode, fd
                    elif line_num == last_parsed_row:
                        # All records in the file have been processed.
                        if 0 == pre_file_inode:
                            # It's the base audit file server_audit.log.
                            fd.close()
                            return cur_file_inode, None
                        else:
                            # All Records in the last file have been
                            # processed, and it's not the base audit file.
                            # So go to the previous one.
                            self.stat_mgr.set_file_inode(pre_file_inode)
                            self.stat_mgr.set_last_parsed_row(0)
                            self.stat_mgr.update_stat()
                            if self.__delete_file and os.path.exists(file_path):
                                os.remove(file_path)
                            break
                    else:
                        logging.error('Line number {} in file {} with '
                                      'inode {} is less than the value {} '
                                      'in the stat file'.format(
                            line_num, file, cur_file_inode, last_parsed_row
                        ))
                        return 0, None
                else:
                    pre_file_inode = cur_file_inode
            if not found_file:
                # If we can not find the file with the target inode, and the
                # audit file list is not changed, the target file is really
                # gone, and incremental synchronization is not possible any
                # longer. If the file list is changed, let's try to find
                # again.
                base_inode_after = os.stat(base_file).st_ino
                if base_inode_after == base_inode_before:
                    logging.error(
                        'Audit file with inode {} not found'.format(
                            self.stat_mgr.get_file_inode()))
                    return 0, None

    def __export_sdb_log(self):
        if 1 == len(self.__buf):
            return
        try:
            self.__buf = self.__buf.encode('UTF-8')
            buf = ""
            checked = False
            dict = {}
            is_msg_info = False
            for ch in self.__buf:
                if True == is_msg_info:
                    buf += ch
                    continue
                if ch != ':':
                    buf += ch
                    continue
                if buf.endswith(FIELD_FROM):
                    value = buf.split(FIELD_FROM)[0].strip()
                    dict[FIELD_USER_NAME] = value
                    buf = ''
                    continue
                elif buf.endswith(FIELD_OBJECT_TYPE):
                    value = int(re.split('[()+]', buf.split()[0])[1])
                    dict[FIELD_RESULT] = int(value)
                    buf = ''
                    continue
                elif buf.endswith(FIELD_RESULT):
                    value = buf.split(FIELD_RESULT)[0].strip()
                    dict[FIELD_ACTION] = value
                    buf = ''
                    continue
                
                value = buf.split()[0].strip()
                if buf.endswith(FIELD_TYPE):
                    checked = True
                    dict[FIELD_TIMESTAMP] = value
                    dt = datetime.datetime.strptime(value,
                                                    "%Y-%m-%d-%H.%M.%S.%f")
                    myTime = int(time.mktime(dt.timetuple())) << 32 | \
                                                       dt.microsecond
                    dict[FIELD_ID] = myTime
                elif buf.endswith(FIELD_PID):
                    checked = True
                    dict[FIELD_TYPE] = value
                elif buf.endswith(FIELD_TID):
                    checked = True
                    dict[FIELD_PID] = int(value)
                elif buf.endswith(FIELD_USER_NAME):
                    checked = True
                    dict[FIELD_TID] = int(value)
                elif buf.endswith(FIELD_ACTION):
                    checked = True
                    dict[FIELD_CLIENT] = value
                elif buf.endswith(FIELD_OBJECT_NAME):
                    checked = True
                    dict[FIELD_OBJECT_TYPE] = value
                elif buf.endswith(FIELD_MESSAGE):
                    if value == self.__cl_full_name:
                        return
                    checked = True
                    is_msg_info = True
                    dict[FIELD_OBJECT_NAME] = value
                if checked:
                    buf = ''
                    checked = False
                    continue
            dict[FIELD_MESSAGE] = buf.strip()
            dict[FIELD_CONNECT_ID] = 0
            dict[FIELD_OPERATION_ID] = 0
            dict[FIELD_ROLE] = self.__role
            dict[FIELD_HOST_NAME] = self.__node_name.split(":")[0].strip()
            dict[FIELD_SVC_NAME] = int(self.__node_name.split(":")[1].strip())

            self.__records.append(dict)
            self.__num_of_records += 1
        except (Exception,ValueError) as e:
            logger.error('Exception: {}, failed to parse log: {}'.format(e, self.__buf))

        if self.__num_of_records >= self.__insert_num:
            self.connect.write_row(self.__records)
            self.stat_mgr.update_stat()
            self.__records = []
            self.__num_of_records = 0

    def __sql_get_operation_type(self, sql):
        if 0 == len(sql):
            return ""
        operation = sql.strip("[ ']").split()[0].lower()
        if operation.startswith("alter") \
                or operation.startswith("create") \
                or operation.startswith("drop") \
                or operation.startswith("declare") \
                or operation.startswith("flush") \
                or operation.startswith("truncate") \
                or operation.startswith("rename"):
            return "DDL"
        elif operation.startswith("call") \
                or operation.startswith("delete") \
                or operation.startswith("do") \
                or operation.startswith("handler") \
                or operation.startswith("insert") \
                or operation.startswith("load") \
                or operation.startswith("replace") \
                or operation.startswith("select") \
                or operation.startswith("update"):
            return "DML"
        elif sql.startswith("select"):
            return "DQL"
        elif operation.startswith("grant") \
                or operation.startswith("revoke"):
            return "DCL"
        else:
            return ""

    def __make_position_id(self, file_inode, row_number):
        # combine inode number and row number into 4 bytes
        # the greater row_number, the more bytes it occupies
        if row_number <= 0xFF:
            file_inode &= 0xFFFFFF
            return ((int(file_inode)) << 8 | row_number)
        elif row_number <= 0xFFFF:
            file_inode &= 0xFFFF
            return ((int(file_inode)) << 16 | row_number)
        elif row_number <= 0xFFFFFF:
            file_inode &= 0xFF
            return ((int(file_inode)) << 24 | row_number)
        else:
            return row_number

    def __export_sql_log(self, line, file_inode, row_number):
        dict = {}
        if 1 == len(line):
            return
        try:
            line = line.encode('UTF-8')
            list = line.split(",")
            my_time = time.mktime(time.strptime(list[0], "%Y%m%d %H:%M:%S"))
            position_id = self.__make_position_id(file_inode, row_number)
            dict[FIELD_ID] = (int(my_time)) << 32 | position_id
            dict[FIELD_ROLE] = self.__role
            dict[FIELD_HOST_NAME] = self.__node_name.split(":")[0].strip()
            dict[FIELD_SVC_NAME] = int(self.__node_name.split(":")[1].strip())
            localtime = time.localtime(my_time)
            dict[FIELD_TIMESTAMP] = time.strftime("%Y-%m-%d-%H.%M.%S.000000",
                                              localtime)
            dict[FIELD_USER_NAME] = list[2]
            dict[FIELD_CLIENT] = socket.gethostbyname(list[3])
            dict[FIELD_PID] = 0
            dict[FIELD_TID] = 0
            dict[FIELD_CONNECT_ID] = int(list[4])
            dict[FIELD_OPERATION_ID] = int(list[5])
            if 0 == len(list[8]):
                dict[FIELD_MESSAGE] = '' 
            else:
                dict[FIELD_MESSAGE] = re.findall(r"'(.*)'", line)[0]
            dict[FIELD_TYPE] = self.__sql_get_operation_type(dict[FIELD_MESSAGE])
            dict[FIELD_ACTION] = list[6]
            dict[FIELD_OBJECT_TYPE] = "DATABASE"
            dict[FIELD_OBJECT_NAME] = list[7]
            dict[FIELD_RESULT] = int(list[-1].strip())

            self.__records.append(dict)
            self.__num_of_records += 1
        except (ValueError,Exception) as e:
            logger.error('Exception: {}, failed to parse log: {}'.format(e, line))

        if self.__num_of_records >= self.__insert_num:
            self.connect.write_row(self.__records)
            self.stat_mgr.update_stat()
            self.__records = []
            self.__num_of_records = 0

    def __export_audit_log_file(self, file_inode, f):
        row_number = 0
        self.stat_mgr.set_file_inode(file_inode)
        lines = f.readlines()
        for origline in lines:
            row_number += 1
            # start from last parse row
            if int(self.stat_mgr.get_last_parsed_row()) >= row_number:
                continue
            self.stat_mgr.set_last_parsed_row(row_number)
            if self.__log_type == LOG_TYPE_SDB:
                self.__buf += origline
                if len(origline) == 1 and origline == "\n":
                    self.__export_sdb_log()
                    self.__buf = ""
            else:
                self.__export_sql_log(origline, file_inode, row_number)
        
        if len(self.__records):
            self.connect.write_row(self.__records)
            self.__records = []
            self.__num_of_records = 0
            self.stat_mgr.update_stat()

    def get_records(self):
        return self.__records 

    def run(self):
        fd = None
        try:
            while True:
                # Find and open the next file which should be processed.
                file_inode, fd = self.__get_next_file()
                if 0 == file_inode:
                    logging.error('Get next audit file failed')
                    sys.exit(1)
                if fd is None:
                    # The file is found, but all operations have been processed.
                    # Let's sleep for a while.
                    time.sleep(PROCESS_WAIT_TIME)
                    continue
                self.__export_audit_log_file(file_inode, fd)
                if fd is not None and not fd.closed:
                    fd.close()
        finally:
            if fd is not None and not fd.closed:
                fd.close()

def run_task(args, work_path):
    """ Start exporter worker """
    global log_exporter

    #ensure cl exists
    connect = SdbConnect(args)
    connect.ensure_cl()

    #init status file
    stat_file = os.path.join(work_path, STATUS_FILE_NAME)
    stat_mgr = StatMgr(stat_file)
    try:
        stat_mgr.load_stat()
    except Exception as err:
        logger.error('Failed to load status: {}'.format(err))
        raise

    #init log reporter
    log_exporter = LogExporter(args, stat_mgr, args.get_log_type(), connect)

    #run log reporter
    log_exporter.run()

def __quit():
    try:
        work_path = os.getcwd()
        pid_file = os.path.join(work_path, PID_FILE_NAME)
        if os.path.exists(pid_file):
            os.remove(pid_file)
        log_exporter.connect.write_row(log_exporter.get_records())
        log_exporter.stat_mgr.update_stat()
        logger.info('Exit')
    except (Exception, ValueError) as e:
        try:
            logger.error('Exception {} occurred during exiting'.format(e))
        except (Exception) as err:
            print('Exception {} occurred during exiting'.format(e))
    sys.exit()

def sig_quit(signum, frame):
    try:
        logger.info('Received signal[{}] and quit'.format(signum))
    except (Exception) as err:
        print('Received signal[{}] and quit'.format(signum))
    __quit()

def main():
    global logger

    work_path = os.getcwd()

    #run logging 
    log_config_file = os.path.join(MY_CONF_PATH, LOG_CONF_FILE_NAME)
    log_instance = Logger()
    rc = log_instance.init(log_config_file)
    if 0 != rc:
        print("[ERROR] Failed to initialize logging: {}".format(rc))
        sys.exit(1)
    logger = log_instance.get_logger()
    logger.info("Start sdbaudit exporter logging")

    # parse arguments
    program = os.path.basename(__file__.replace(".py", ""))
    optMgr = OptionsMgr(program)
    rc = optMgr.parse_option()
    if 0 != rc:
        return rc

    # prepare for pid file
    pid_file = os.path.join(work_path, PID_FILE_NAME)
    if os.path.exists(pid_file):
        with open(pid_file, "r") as f:
            pid = str(f.readline())
        if os.path.exists("/proc/{pid}".format(pid=pid)):
            with open("/proc/{pid}/cmdline".format(pid=pid), "r") as process:
                process_info = process.readline()
            if process_info.find(sys.argv[0]) != -1:
                logger.info("Only one sdbaudit exporter process is allowed to run " \
                      "at the same time. Exit...")
                return 1
    with open(pid_file, "w") as f:
        pid = str(os.getpid())
        f.write(pid)

    # Signal handler, make it write records and write status file
    # when ctrl + c is pressed.
    signal.signal(signal.SIGINT, sig_quit)
    signal.signal(signal.SIGTERM, sig_quit)

    # run
    try:
        run_task(optMgr.args, work_path)
    except Exception as error:
        logger.error('Failed to run task:' + str(error))
    __quit()



logger = None
log_exporter = None

if __name__ == '__main__':
    rc = main()
    sys.exit(rc)

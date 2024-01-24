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
import time
import signal
import optparse
from sdbaudit_exprt import pid_exist
from version import get_version, get_release, get_git_version, get_build_time

try:
    reload(sys)
    sys.setdefaultencoding('utf8')
except NameError:
    import importlib,sys
    importlib.reload(sys)

DESCRIPTION = '''%prog is a daemon for sdbaudit_exprt.'''

USAGE = '''%prog <--start>|<--stop>|<--status>
       %prog --help'''

LOG_TYPE_MYSQL = 'mysql'
LOG_TYPE_SDB = 'sdb'
LOG_TYPE_MARIADB = 'mariadb'

MY_HOME = os.path.abspath(os.path.dirname(__file__))
VERSION_FILE_NAME = "version.info"
PID_FILE_NAME = "sdbaudit_daemon.pid"
EXPPRTER_PID_FILE_NAME = "sdbaudit.pid"
MY_CONF_PATH = os.path.join(MY_HOME, 'conf')


class Daemon:
    def __init__(self, pid_file, stdin='/dev/null', stdout='/dev/null',
                 stderr='/dev/null'):
        self.stdin = stdin
        self.stdout = stdout
        self.stderr = stderr
        self.pid_file = pid_file

    def daemonize(self):
        try:
            pid = os.fork()
            if pid > 0:
                sys.exit(0)
        except OSError as e:
            sys.stderr.write('fork child process failed.')
            sys.exit(1)
        os.chdir('/')
        os.setsid()
        os.umask(0)
        sys.stdout.flush()
        sys.stderr.flush()
        si = open(self.stdin, 'r')
        so = open(self.stdout, 'a+')
        se = open(self.stderr, 'a+')
        os.dup2(si.fileno(), sys.stdin.fileno())
        os.dup2(so.fileno(), sys.stdout.fileno())
        os.dup2(se.fileno(), sys.stderr.fileno())
        with open(self.pid_file, "w") as f:
            pid = str(os.getpid())
            f.write(pid)

    def run(self):
        ctl_exec = os.path.join(MY_HOME, "sdbaudit_ctl.py")
        while True:
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
                    pid_file = os.path.join(node_path, 
                                            EXPPRTER_PID_FILE_NAME)
                    if not os.path.exists(pid_file):
                        continue
                    with open(pid_file, 'r') as f:
                        pid = int(f.readline())
                    if not pid_exist(pid):
                        os.system("nohup python {} start > /dev/null " \
                                  "2>&1 &".format(ctl_exec))
            time.sleep(1)

class OptionsMgr:
    def __init__(self, program):
        self.__parser = optparse.OptionParser(
            description = DESCRIPTION,
            usage = USAGE,
            prog = program
        )
        self.opt = None

    def __add_option(self):
        #Add --start option
        self.__parser.add_option("--start", action='store_true', dest='start',
                                 help="start the daemon.")
        #Add --stop option
        self.__parser.add_option("--stop", action='store_true', dest='stop',
                                 help="stop the daemon.")
        #Add --status option
        self.__parser.add_option("--status", action='store_true', dest='status',
                                 help="check if daemon exists")

				#Add --version option
        self.__parser.add_option("-v", "--version", action='store_true',
                                 dest="version", help="show version")

    def __show_version(self):
        print("sdbaudit: {}".format(get_version())) 
        print("Release: {}".format(get_release())) 
        print("Git version: {}".format(get_git_version())) 
        print(get_build_time())
        sys.exit(0)
        
    def parse_option(self):
        self.__add_option()
        self.opt,args = self.__parser.parse_args()
        if 0 != len(args):
            print("[ERROR] Invalid argument({}). Try 'python {} -h' for more " \
                  "information".format(args, sys.argv[0]))
            return 1
        if self.opt.version:
            self.__show_version()
        return 0

class Worker:
    def __init__(self, optMgr):
        self.opt = optMgr.opt

    def __start(self):
        pid_file = os.path.join(MY_HOME, PID_FILE_NAME)
        if os.path.exists(pid_file):
            with open(pid_file, "r") as f:
                pid = str(f.readline())
            if os.path.exists("/proc/{pid}".format(pid=pid)):
                with open("/proc/{pid}/cmdline".format(pid=pid), "r") as process:
                    process_info = process.readline()
                if process_info.find(sys.argv[0]) != -1:
                    print("[INFO] sdbaudit_daemon process has been already " \
                          "started")
                    return 1
        daemon = Daemon(pid_file)
        daemon.daemonize()
        daemon.run()

    def __stop(self):
        pid_file = os.path.join(MY_HOME, PID_FILE_NAME)
        if os.path.exists(pid_file):
            with open(pid_file, "r") as f:
                pid = int(f.readline())
                if not pid_exist(pid):
                    print("[INFO] sdbaudit_daemon process is not running.")
                    return 1
                else:
                    os.kill(pid, 15)
                    os.remove(pid_file)
        return 0

    def __status(self):
        pid_file = os.path.join(MY_HOME, PID_FILE_NAME)
        if os.path.exists(pid_file):
            with open(pid_file, "r") as f:
                pid = int(f.readline())
                if pid_exist(pid):
                    print("sdbaudit_daemon process(PID: {}) is " \
                          "running.".format(pid))
                    return 0
        print("sdbaudit_daemon process is not running.")
        return 0
        
    def run_command(self):
        if self.opt.start:
            rc = self.__start()
        elif self.opt.stop:
            rc = self.__stop()
        elif self.opt.status:
            rc = self.__status()
        return rc

def main():
    #Setup the command parser
    program = os.path.basename(__file__.replace(".py", ""))
    optMgr = OptionsMgr(program)
    rc = optMgr.parse_option()
    if 0 != rc: 
      return rc
    work = Worker(optMgr)
    rc = work.run_command()
    return rc

if __name__ == '__main__':
    rc = main()
    sys.exit(rc)

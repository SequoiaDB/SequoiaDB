#coding=utf-8
############################################
# Decription:
#   Compile db module and package into a tar file
############################################

import os,sys
import platform
import shutil
import argparse
import paramiko
import glob
import tarfile
import codecs
from subprocess import Popen, PIPE
from scp import SCPClient
sys.path.append(os.path.join(os.path.abspath(os.path.dirname(__file__)), 'script'))
from build_base_module import CompileBaseModuleMgr

ROOT_DIR = os.path.abspath(os.path.dirname(__file__))
# remote CI machine, for build doc
REMOTE_HOST = '192.168.30.202'
REMOTE_USER = 'sequoiadb'
REMOTE_PASSWD = 'jenkins'
REMOTE_PORT = 22
# os
OS_TYPE = platform.system()
OS_ARCH = platform.machine()
# log
LOG_FILE = None

class OptionsMgr:
   def __init__(self):
      self.__parser = argparse.ArgumentParser()
      compile_group = self.__parser.add_argument_group('compile arguments')
      compile_group.add_argument(
         '--enterprise', action = 'store_true', default = False,
         help = 'Compile enterprise version'
      )
      compile_group.add_argument(
         '--release', action = 'store_true', default = False,
         help = 'Compile release version'
      )
      compile_group.add_argument(
         '--dd', action = 'store_true', default = False,
         help = 'Compile debug version'
      )
      compile_group.add_argument(
         '--hybrid', action = 'store_true', default = False,
         help = 'Compile hybrid version'
      )
      compile_group.add_argument(
         '--language', metavar = 'language', type = str,
         help = 'Compile language, onyl support en for now'
      )
      compile_group.add_argument(
         '-j', '--job', metavar = 'jobNum', type = int, default = 4,
         help = 'Compile thread number, default: 4'
      )
      compile_group.add_argument(
         '--enable-windows-compile', action = 'store_true', default = False,
         help = 'Whether enable compile in windows'
      )
      compile_group.add_argument(
         '--db-path', metavar = 'dbPath', type = str,
         help = 'DB path in windows'
      )
      compile_group.add_argument(
         '--coverage', action = 'store_true', default = False,
         help = 'Whether generate coverage information'
      )
      compile_group.add_argument(
         '--clean', action = 'store_true', default = False,
         help = "Whether clean all project, all changes at local will be lost!"
      )
      build_group = self.__parser.add_argument_group('build arguments')
      build_group.add_argument(
         '--install-dir', type = str, metavar = 'installDir',
         help = 'Install Dir'
      )
      build_group.add_argument(
         '--logfile', type = str, metavar = 'logFile',
         help = 'The log file'
      )

   def parse_args(self):
      self.args = self.__parser.parse_args()
      # when enable windows compile, need the db path in windows
      if self.args.enable_windows_compile and self.args.db_path == None:
         err_exit(1, 'Parse args fail, need db path in windows when enable windows compile')

   def get_compile_args(self):
      compile_args = []
      compile_args.append('-j {}'.format(self.args.job))
      
      if self.args.enterprise:
         compile_args.append('--enterprise')

      if self.args.dd:
         compile_args.append('--dd')

      if self.args.release:
         compile_args.append('--release')

      if self.args.hybrid:
         compile_args.append('--hybrid')

      if self.args.language:
         compile_args.append('--language {}'.format(self.args.language))

      if self.args.coverage:
         compile_args.append('--cov')

      return compile_args

   def get_debug(self):
      return self.args.dd

   def get_enterprise(self):
      return self.args.enterprise
   
   def get_hybrid(self):
      return self.args.hybrid

   def get_job_num(self):
      return self.args.job

   def get_enable_windows_compile(self):
      return self.args.enable_windows_compile

   def get_db_path(self):
      return self.args.db_path

   def get_install_dir(self):
      return self.args.install_dir

   def get_jdk(self):
      return self.args.jdk

   def get_logfile(self):
      return self.args.logfile

   def get_coverage(self):
      return self.args.coverage

   def get_clean(self):
      return self.args.clean

class RemoteMgr():
   def __init__(self, opt_mgr):
      self.client = paramiko.SSHClient()
      # setting authentication with auto
      self.client.set_missing_host_key_policy(paramiko.AutoAddPolicy)
      self.client.connect(
         hostname = REMOTE_HOST,
         port = REMOTE_PORT,
         username = REMOTE_USER,
         password = REMOTE_PASSWD
      )
      self.scp = SCPClient(self.client.get_transport())
      self.db_path = opt_mgr.get_db_path()
      self.job = opt_mgr.get_job_num()

   def remote_exec_compile(self):
      print_log('Begine remote compile')
      script_name = os.path.join(self.db_path, 'script/build_ex_module.py')
      build_cmd = 'python {} -j {}'.format(script_name, self.job)
      stdin, stdout, stderr = self.client.exec_command(build_cmd, get_pty=True)
      while not stdout.channel.exit_status_ready():
         data = stdout.readline()
         print_log(data)
      rs = stdout.channel.recv_exit_status()
      err_exit(rs, 'Run command {} fail in {}, remote compile fail'.format(build_cmd, REMOTE_HOST))
      print_log('Finish remote compile')

   def get_remote_file(self):
      print_log('Begine scp from remote computer')
      ex_module_file = os.path.join(ROOT_DIR, 'ex_module')
      ex_module_tar_file = ex_module_file + '.tar.gz'
      # if ex_module file exists, remvoe it
      if os.path.exists(ex_module_file):
         shutil.rmtree(ex_module_file)
      if os.path.exists(ex_module_tar_file):
         os.remove(ex_module_tar_file)
      # scp file to the local and uncompress
      remote_scp_file = os.path.join(self.db_path, 'ex_module.tar.gz')
      self.scp.get(remote_scp_file, ROOT_DIR)
      tar = tarfile.open(ex_module_tar_file, 'r:gz')
      tar.extractall(path = ROOT_DIR)
      tar.close()
      print_log('Finish scp from remote computer')

   def __del__(self):
      self.client.close()

# for file operate
def copy_file(src, target):
   files = glob.glob(src)
   for file in files:
      cmd = 'cp -r {} {}'.format(file, target)
      run_command(cmd)

def remove_file(src):
   if os.path.exists(src):
      print_log('Remove file {}'.format(src))
      if os.path.isdir(src):
         shutil.rmtree(src)
      else:
         os.remove(src)

# chmod permission, only in this level
def chmod(src, privilege):
   files = glob.glob(src)
   for file in files:
      cmd = 'chmod {} {}'.format(privilege, file)
      run_command(cmd)

def setcap(src):
   cmd = 'sudo setcap CAP_SYS_RESOURCE=+ep {}'.format(src)
   run_command(cmd)

# for log operate
def init_log(log_file):
   global LOG_FILE
   LOG_FILE = log_file
   # remove the old log file
   if LOG_FILE is not None and os.path.exists(LOG_FILE):
      os.remove(LOG_FILE)

def print_log(log):
   if LOG_FILE:
      # we need to write log data from windows, in paramiko connection, all return data has been
      # encode with utf-8, so we need to open file with encoding utf-8
      file = codecs.open(LOG_FILE, 'a', encoding='utf-8')
      if not isinstance(log, unicode):
         log = unicode(log, 'utf-8')
      file.write(log)
      file.write('\n')
      file.flush()
      file.close()
   else:
      print(log)

# for package operate
def package_db(opt_mgr, ver):
   print_log('Begine package db')
   install_dir = os.path.join(opt_mgr.get_install_dir(), 'sequoiadb')
   # remove file before install and package
   remove_file(install_dir)
   # create dir in install dir
   dirs = ['bin', 'conf/samples', 'conf/local', 'conf/log', 'doc', 'include', 'java/jdk',
            'lib', 'license', 'packet', 'postgresql', 'python', 'samples', 'tools/server/php',
            'tools/sequoias3', 'tools/sequoias3/java', 'tools/sequoiafs', 'tools/upgrade', 'web',
            'www', 'spark', 'flink', 'plugins', 'plugins/SequoiaSQL', 'lib/phplib', 'CSharp',
            'tools/script']
   for dir in dirs:
      os.makedirs(os.path.join(install_dir, dir))

   # begine copy
   copy_file(os.path.join(ROOT_DIR, 'bin'), install_dir)
   copy_file(os.path.join(ROOT_DIR, 'script/sdbwsart'), os.path.join(install_dir, 'bin'))
   copy_file(os.path.join(ROOT_DIR, 'script/sdbwstop'), os.path.join(install_dir, 'bin'))
   copy_file(os.path.join(ROOT_DIR, 'client/include'), install_dir)
   copy_file(os.path.join(ROOT_DIR, 'client/lib'), install_dir)
   copy_file(os.path.join(ROOT_DIR, 'client/samples'), install_dir)
   copy_file(os.path.join(ROOT_DIR, 'SequoiaDB/web'), install_dir)
   copy_file(os.path.join(ROOT_DIR, 'conf/script'), os.path.join(install_dir, 'conf'))
   copy_file(os.path.join(ROOT_DIR, 'conf/samples'), os.path.join(install_dir, 'conf'))
   copy_file(os.path.join(ROOT_DIR, 'doc/manual'), os.path.join(install_dir, 'doc'))
   copy_file(os.path.join(ROOT_DIR, 'tools/om_plugins/sequoiasql/bin'), os.path.join(install_dir, 'plugins/SequoiaSQL'))
   copy_file(os.path.join(ROOT_DIR, 'tools/om_plugins/sequoiasql/www'), os.path.join(install_dir, 'plugins/SequoiaSQL'))
   copy_file(os.path.join(ROOT_DIR, 'SequoiaDB/engine/tools/sequoias3/config'), os.path.join(install_dir, 'tools/sequoias3'))
   copy_file(os.path.join(ROOT_DIR, 'SequoiaDB/engine/tools/sequoias3/sample'), os.path.join(install_dir, 'tools/sequoias3'))
   copy_file(os.path.join(ROOT_DIR, 'SequoiaDB/engine/tools/sequoias3/driver/java/target/lib'), os.path.join(install_dir, 'tools/sequoias3/java'))
   copy_file(os.path.join(ROOT_DIR, 'SequoiaDB/engine/tools/sequoiafs/bin'), os.path.join(install_dir, 'tools/sequoiafs'))
   copy_file(os.path.join(ROOT_DIR, 'SequoiaDB/engine/tools/sequoiafs/conf'), os.path.join(install_dir, 'tools/sequoiafs'))
   copy_file(os.path.join(ROOT_DIR, 'tools/sequoiafs/bin/*'), os.path.join(install_dir, 'tools/sequoiafs/bin'))
   copy_file(os.path.join(ROOT_DIR, 'tools/sdbsupport'), os.path.join(install_dir, 'tools'))
   copy_file(os.path.join(ROOT_DIR, 'tools/deploy'), os.path.join(install_dir, 'tools'))
   copy_file(os.path.join(ROOT_DIR, 'tools/expect'), os.path.join(install_dir, 'tools'))
   copy_file(os.path.join(ROOT_DIR, 'tools/dr_ha'), os.path.join(install_dir, 'tools'))
   copy_file(os.path.join(ROOT_DIR, 'tools/ptmallocstats'), os.path.join(install_dir, 'tools'))
   copy_file(os.path.join(ROOT_DIR, 'tools/crontask'), os.path.join(install_dir, 'tools'))
   copy_file(os.path.join(ROOT_DIR, 'tools/sdbmigrate'), os.path.join(install_dir, 'tools'))
   copy_file(os.path.join(ROOT_DIR, 'tools/sdbaudit'), os.path.join(install_dir, 'tools'))
   copy_file(os.path.join(ROOT_DIR, 'tools/generateJSforcopycluster'), os.path.join(install_dir, 'tools'))
   copy_file(os.path.join(ROOT_DIR, 'tools/upgrade'), os.path.join(install_dir, 'tools'))
   copy_file(os.path.join(ROOT_DIR, 'client/admin/admintpl/*'), os.path.join(install_dir, 'www'))
   copy_file(os.path.join(ROOT_DIR, 'java/openJDK-8u292'), os.path.join(install_dir, 'java/jdk'))
   copy_file(os.path.join(ROOT_DIR, 'tools/sdbmemcheck'), os.path.join(install_dir, 'tools'))
   copy_file(os.path.join(ROOT_DIR, 'conf/*.conf'), os.path.join(install_dir, 'conf'))
   copy_file(os.path.join(ROOT_DIR, 'driver/spark/target/*.jar'), os.path.join(install_dir, 'spark'))
   copy_file(os.path.join(ROOT_DIR, 'driver/spark-3.0/target/*.jar'), os.path.join(install_dir, 'spark'))
   copy_file(os.path.join(ROOT_DIR, 'driver/flink/target/sdb-flink-connector-*.jar'), os.path.join(install_dir, 'flink'))
   copy_file(os.path.join(ROOT_DIR, 'tools/om_plugins/sequoiasql/source/target/*jar'), os.path.join(install_dir, 'plugins/SequoiaSQL/bin'))
   copy_file(os.path.join(ROOT_DIR, 'SequoiaDB/engine/tools/sequoias3/target/sequoia*.jar'), os.path.join(install_dir, 'tools/sequoias3'))
   copy_file(os.path.join(ROOT_DIR, 'SequoiaDB/engine/tools/sequoias3/sequoias3.sh'), os.path.join(install_dir, 'tools/sequoias3'))
   copy_file(os.path.join(ROOT_DIR, 'SequoiaDB/engine/tools/sequoias3/README.txt'), os.path.join(install_dir, 'tools/sequoias3'))
   copy_file(os.path.join(ROOT_DIR, 'SequoiaDB/engine/tools/sequoias3/driver/java/target/sequoias3-client*.jar'), os.path.join(install_dir, 'tools/sequoias3/java'))
   copy_file(os.path.join(ROOT_DIR, 'driver/postgresql/sdb_fdw.so'), os.path.join(install_dir, 'postgresql'))
   copy_file(os.path.join(ROOT_DIR, 'driver/postgresql/sdb_fdw--1.0.sql'), os.path.join(install_dir, 'postgresql'))
   copy_file(os.path.join(ROOT_DIR, 'driver/postgresql/sdb_fdw.control'), os.path.join(install_dir, 'postgresql'))
   copy_file(os.path.join(ROOT_DIR, 'driver/python/pysequoiadb*py*.tar.gz'), os.path.join(install_dir, 'python'))
   copy_file(os.path.join(ROOT_DIR, 'driver/java/target/sequoiadb*.jar'), os.path.join(install_dir, 'java'))
   copy_file(os.path.join(ROOT_DIR, 'script/sequoiadb'), install_dir)
   copy_file(os.path.join(ROOT_DIR, 'script/install_om.sh'), install_dir)
   copy_file(os.path.join(ROOT_DIR, 'script/compatible.sh'), install_dir)
   copy_file(os.path.join(ROOT_DIR, 'script/preUninstall.sh'), install_dir)
   copy_file(os.path.join(ROOT_DIR, 'script/om_ver.conf'), install_dir)
   copy_file(os.path.join(ROOT_DIR, 'script/version.conf'), install_dir)
   copy_file(os.path.join(ROOT_DIR, 'script/sdbcm.service'), install_dir)
   copy_file(os.path.join(ROOT_DIR, 'script/generate_version_file.sh'), os.path.join(install_dir, 'tools/script'))
   copy_file(os.path.join(ROOT_DIR, 'script/install_file_changes.sh'), os.path.join(install_dir, 'tools/script'))
   copy_file(os.path.join(ROOT_DIR, 'script/service_control.sh'), os.path.join(install_dir, 'tools/script'))
   copy_file(os.path.join(ROOT_DIR, 'driver/C#.Net/build/release/sequoiadb.dll'), os.path.join(install_dir, 'CSharp'))
   # copy the php base on system os or arch
   if OS_ARCH == 'aarch64':
      copy_file(os.path.join(ROOT_DIR, 'tools/server/php_arm/*'), os.path.join(install_dir, 'tools/server/php'))
   elif OS_TYPE == 'Linux':
      copy_file(os.path.join(ROOT_DIR, 'tools/server/php_linux/*'), os.path.join(install_dir, 'tools/server/php'))

   # copy php driver base on compile type
   if opt_mgr.get_debug():
      copy_file(os.path.join(ROOT_DIR, 'driver/php/build/dd/*.so'), os.path.join(install_dir, 'lib/phplib'))
   else:
      copy_file(os.path.join(ROOT_DIR, 'driver/php/build/normal/*.so'), os.path.join(install_dir, 'lib/phplib'))

   # copy license base on compile type
   if opt_mgr.get_enterprise():
      copy_file(os.path.join(ROOT_DIR, 'licenses/license_en.txt'), os.path.join(install_dir, 'license'))
      copy_file(os.path.join(ROOT_DIR, 'licenses/license_zh.txt'), os.path.join(install_dir, 'license'))
   else:
      copy_file(os.path.join(ROOT_DIR, 'licenses/license_free_en.txt'), os.path.join(install_dir, 'license'))
      copy_file(os.path.join(ROOT_DIR, 'licenses/license_free_zh.txt'), os.path.join(install_dir, 'license'))
      os.rename(os.path.join(install_dir, 'license/license_free_en.txt'), os.path.join(install_dir, 'license/license_en.txt'))
      os.rename(os.path.join(install_dir, 'license/license_free_zh.txt'), os.path.join(install_dir, 'license/license_zh.txt'))
   if OS_ARCH == 'x86_64':
      remove_file(os.path.join(install_dir, 'tools/expect/bin/expect_arm'))
   elif OS_ARCH == 'aarch64':
      remove_file(os.path.join(install_dir, 'tools/expect/bin/expect'))
      os.rename(os.path.join(install_dir, 'tools/expect/bin/expect_arm'), os.path.join(install_dir, 'tools/expect/bin/expect'))

   chmod(os.path.join(install_dir, 'bin/sdbwsart'), 'u+x')
   chmod(os.path.join(install_dir, 'bin/sdbwstop'), 'u+x')
   chmod(os.path.join(install_dir, 'postgresql/*sdb_fdw.so*'), 'a+x')
   chmod(os.path.join(install_dir, 'install_om.sh'), '0755')
   chmod(os.path.join(install_dir, 'plugins/SequoiaSQL/bin/*.sh'), '0755')
   chmod(os.path.join(install_dir, 'conf/script/*.sh'), '0755')
   chmod(os.path.join(install_dir, 'conf/script/*/*.sh'), '0755')
   chmod(os.path.join(install_dir, 'www/shell/*'), '0755')
   chmod(os.path.join(install_dir, 'tools/server/php/bin/*'), '0755')
   chmod(os.path.join(install_dir, 'tools/sequoiafs/bin/*'), 'u+x')
   chmod(os.path.join(install_dir, 'tools/dr_ha/*.sh'), 'u+x')
   chmod(os.path.join(install_dir, 'tools/sdbmigrate/bin/*.sh'), 'u+x')
   chmod(os.path.join(install_dir, 'tools/upgrade/*.sh'), 'u+x')
   chmod(os.path.join(install_dir, 'tools/sequoias3/sequoias3.sh'), 'u+x')
   chmod(os.path.join(install_dir, 'tools/deploy/quickDeploy.sh'), '755')
   chmod(os.path.join(install_dir, 'tools/deploy/postgresql.conf'), '666')
   chmod(os.path.join(install_dir, 'tools/deploy/mysql.conf'), '666')
   chmod(os.path.join(install_dir, 'tools/deploy/sequoiadb.conf'), '644')
   chmod(os.path.join(install_dir, 'tools/sdbmemcheck'), 'u+x')
   chmod(os.path.join(install_dir, 'tools/expect/bin/expect'), 'u+x')
   chmod(os.path.join(install_dir, 'tools/expect/trust/trustRelpConf.sh'), 'u+x')
   chmod(os.path.join(install_dir, 'tools/sdbsupport/sdbsupport.sh'), 'u+x')
   chmod(os.path.join(install_dir, 'tools/ptmallocstats/ptmalloc_stats.py'), 'u+x')
   chmod(os.path.join(install_dir, 'tools/crontask/sdbtaskctl'), 'u+x')
   chmod(os.path.join(install_dir, 'tools/crontask/sdbtaskdaemon'), 'u+x')
   chmod(os.path.join(install_dir, 'tools/sdbaudit/sdbaudit_ctl.py'), 'u+x')
   chmod(os.path.join(install_dir, 'tools/sdbaudit/sdbaudit_daemon.py'), 'u+x')
   chmod(os.path.join(install_dir, 'preUninstall.sh'), 'a+x')
   chmod(os.path.join(install_dir, 'bin/sdbomtool'), 'u+s')
   chmod(os.path.join(install_dir, 'tools/script/generate_version_file.sh'), 'u+x')
   chmod(os.path.join(install_dir, 'tools/script/service_control.sh'), 'u+x')

   # setcap
   setcap(os.path.join(install_dir, 'bin/sdbstart'))
   setcap(os.path.join(install_dir, 'bin/sdbcmart'))

   generate_version_file(install_dir, opt_mgr, ver)

   # make tar file
   tar_file = install_dir + '.tar.gz'
   tar = tarfile.open(tar_file, 'w:gz')
   tar.add(install_dir, arcname = os.path.basename(install_dir))
   tar.close()
   print_log('Finish package db')

def generate_version_file(file_path, opt_mgr, ver):
   file = open(os.path.join(file_path, 'VERSION'), 'a')
   file.write('SequoiaDB version: {}\n'.format(ver.get_version()))
   file.write('Release: {}\n'.format(ver.get_release()))
   file.write('Git version: {}\n'.format(ver.get_git_version()))
   file.write(ver.get_build_time())
   if opt_mgr.get_enterprise():
      if opt_mgr.get_debug():
         if opt_mgr.get_hybrid():
            file.write('(Enterprise Hybrid Debug)')
         else:
            file.write('(Enterprise Debug)')
      else:
         if opt_mgr.get_hybrid():
            file.write('(Enterprise Hybrid)')
         else:
            file.write('(Enterprise)')
   else:
      if opt_mgr.get_debug():
         file.write('(Debug)')
   file.flush()
   file.close()

def package_bin(opt_mgr, ver):
   print_log('Begine package bin dir')
   bin_name = 'sequoiadb-{}-linux_{}'.format(ver.get_version(), OS_ARCH)
   if opt_mgr.get_enterprise():
      bin_name += '-enterprise'
   if opt_mgr.get_hybrid():
      bin_name += '-hybrid'
   bin_name += '-bin.tar.gz'
   tar_name = os.path.join(opt_mgr.get_install_dir(), bin_name)
   tar = tarfile.open(tar_name, 'w:gz')
   tar.add(os.path.join(opt_mgr.get_install_dir(), 'sequoiadb/bin'), arcname = 'bin')
   tar.close()
   print_log('Finish package bin dir')

def package_all_driver(opt_mgr, ver):
   print_log('Begine package full driver')
   install_dir = os.path.join(opt_mgr.get_install_dir(), 'driver')
   remove_file(install_dir)
   os.makedirs(install_dir)
   dirs = ['C#', 'C&CPP', 'Java', 'PHP', 'Postgresql', 'Python', 'Spark', 'Flink']
   for dir in dirs:
      os.makedirs(os.path.join(install_dir, dir))
   copy_file(os.path.join(ROOT_DIR, 'driver/C#.Net/build/release/sequoiadb.dll'), os.path.join(install_dir, 'C#'))
   copy_file(os.path.join(ROOT_DIR, 'driver/spark/target/*.jar'), os.path.join(install_dir, 'Spark'))
   copy_file(os.path.join(ROOT_DIR, 'driver/spark-3.0/target/*.jar'), os.path.join(install_dir, 'Spark'))
   copy_file(os.path.join(ROOT_DIR, 'driver/flink/target/sdb-flink-connector-*.jar'), os.path.join(install_dir, 'Flink'))
   copy_file(os.path.join(ROOT_DIR, 'driver/java/target/sequoiadb*.jar'), os.path.join(install_dir, 'Java'))
   if opt_mgr.get_debug():
      copy_file(os.path.join(ROOT_DIR, 'driver/php/build/dd/*.so'), os.path.join(install_dir, 'PHP'))
   else:
      copy_file(os.path.join(ROOT_DIR, 'driver/php/build/normal/*.so'), os.path.join(install_dir, 'PHP'))
   copy_file(os.path.join(ROOT_DIR, 'driver/python/pysequoiadb*py*.tar.gz'), os.path.join(install_dir, 'Python'))
   copy_file(os.path.join(ROOT_DIR, 'driver/postgresql/sdb_fdw.so'), os.path.join(install_dir, 'Postgresql'))
   copy_file(os.path.join(ROOT_DIR, 'driver/postgresql/sdb_fdw--1.0.sql'), os.path.join(install_dir, 'Postgresql'))
   copy_file(os.path.join(ROOT_DIR, 'driver/postgresql/sdb_fdw.control'), os.path.join(install_dir, 'Postgresql'))
   chmod(os.path.join(install_dir, 'postgresql/*sdb_fdw.so*'), 'a+x')

   # c&cpp contail special character, use shutil to copy
   shutil.copytree(os.path.join(ROOT_DIR, 'client/include'), os.path.join(install_dir, 'C&CPP/include'))
   shutil.copytree(os.path.join(ROOT_DIR, 'client/lib'), os.path.join(install_dir, 'C&CPP/lib'))
   
   tar_name = 'sequoiadb-driver-{}-linux_{}.tar.gz'.format(ver.get_version(), OS_ARCH)
   tar = tarfile.open(os.path.join(opt_mgr.get_install_dir(), tar_name), 'w:gz')
   tar.add(install_dir, arcname = os.path.basename(install_dir))
   tar.close()
   print_log('Finish package full driver')

def package_driver(opt_mgr, ver):
   print_log('Begine package each driver')
   # already have driver in install dir, let rename each driver and make tar
   file_suffix = '-{}-linux_{}'.format(ver.get_version(), OS_ARCH)
   dirs = ['C#', 'C&CPP', 'Java', 'PHP', 'Postgresql', 'Python', 'Spark', 'Flink']
   driver_dir = os.path.join(opt_mgr.get_install_dir(), 'driver')
   for dir in dirs:
      file_name = dir + file_suffix
      tar_name = file_name + '.tar.gz'
      tar = tarfile.open(os.path.join(opt_mgr.get_install_dir(), tar_name), 'w:gz')
      tar.add(os.path.join(driver_dir, dir), file_name)
      tar.close()
   print_log('Finish package each driver')
   
def package_doc(opt_mgr):
   print_log('Begine package doc')
   # copy into install dir, not need to package
   install_dir = opt_mgr.get_install_dir()
   copy_file(os.path.join(ROOT_DIR, 'ex_module/SequoiaDB_usermanuals_v*.chm'), install_dir)
   copy_file(os.path.join(ROOT_DIR, 'ex_module/SequoiaDB_usermanuals_v*.pdf'), install_dir)
   copy_file(os.path.join(ROOT_DIR, 'ex_module/SequoiaDB_usermanuals_v*.tar.gz'), install_dir)
   print_log('Finish package doc')

def check_env():
   run_command('python2 --version')
   run_command('python3 --version')
   # need postgresql to compile pg connector
   run_command('pg_config --version')
   run_command('gcc --version')
   run_command('ant -version')
   run_command('make --version')
   run_command('scons --version')
   run_command('mvn --version')

def err_exit(rs, err_msg):
   if rs != 0:
      print_log('Return code: {}'.format(rs))
      print_log(err_msg)
      sys.exit(1)

def run_command(cmd):
   print_log('Run command: {}'.format(cmd))
   process = Popen(cmd, shell=True, stdout=PIPE, stderr=PIPE)
   out, err = process.communicate()
   if 0 == process.returncode:
      print_log(out)
   else:
      print_log(err)
      err_msg = 'Run command {} fail'.format(cmd)
      err_exit(process.returncode, err_msg)

def main():
   global LOG_FILE
   # parse args
   opt_mgr = OptionsMgr()
   opt_mgr.parse_args()
   init_log(opt_mgr.get_logfile())
   if opt_mgr.get_clean():
      run_command('git clean -fxd')

   # if compile on windows, no need to check env
   if opt_mgr.get_enable_windows_compile():
      remote = RemoteMgr(opt_mgr)
      remote.remote_exec_compile()
      remote.get_remote_file()
      if opt_mgr.get_install_dir():
         package_doc(opt_mgr)
      sys.exit(0)

   check_env()

   # compile base module
   compile_base_mgr = CompileBaseModuleMgr(opt_mgr)
   # base module need jdk, download it
   compile_base_mgr.download_jdk('jdk')
   compile_base_mgr.compile_base_module()
   # in the first build, version.py didnot include version info, so wo need to build base module
   # after import version
   sys.path.append(os.path.join(os.path.abspath(os.path.dirname(__file__)), 'misc/autogen'))
   import version as ver
   db_version = ver.get_version()
   compile_base_mgr.compile_driver(db_version)
   compile_base_mgr.compile_connector(db_version)

   if opt_mgr.get_install_dir():
      package_db(opt_mgr, ver)
      package_bin(opt_mgr, ver)
      package_all_driver(opt_mgr, ver)
      package_driver(opt_mgr, ver)

if __name__ == "__main__":
   sys.exit(main())

############################################
# Decription:
#   Compile source code, include:
#   engineer, driver, connector
############################################
import os,sys
import platform
import shutil
import argparse
import codecs
from subprocess import Popen, PIPE

OS_TYPE = platform.system()
LOG_FILE = None

class CompileBaseModuleMgr:
   def __init__(self, opt_mgr):
      global LOG_FILE
      LOG_FILE = opt_mgr.get_logfile()
      self.root_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir))
      self.cur_dir = os.path.abspath(os.path.dirname(__file__))
      self.opt_mgr = opt_mgr
      self.job = opt_mgr.get_job_num()
      self.debug = opt_mgr.get_debug()

   def download_jdk(self, jdk_version):
      jdk_url = 'http://gitlab.sequoiadb.com/sequoiadb/jdk/raw/master/installJDK.sh'
      jdk_script_name = 'installJDK.sh'
      install_path = os.path.join(self.root_dir, 'java')
      print_log('Begine download jdk')
      if not os.path.exists(os.path.join(self.root_dir, jdk_script_name)):
         # put in root dir
         self.run_in_dir('wget {}'.format(jdk_url), self.root_dir)
         self.run_in_dir('chmod u+x {}'.format(jdk_script_name), self.root_dir)
      # need download jdk for compile, lets put in java dir
      if jdk_version == 'jdk7':
         download_cmd = 'bash {} --installDir {} --type orcaleJDK --version 7u80'.format(jdk_script_name, install_path)
         java_home = os.path.join(install_path, 'orcaleJDK-7u80')
         remove_file(java_home)
         self.run_in_dir(download_cmd, self.root_dir)
         jdk7_env = os.environ.copy()
         jdk7_env['JAVA_HOME'] = java_home
         jdk7_env['PATH'] = '{}/bin:{}'.format(java_home, jdk7_env['PATH'])
         jdk7_env['CLASSPATH'] = '.:{}/lib/dt.jar:{}/lib/tools.jar'.format(java_home, java_home)
         self.jdk7_env = jdk7_env
         self.run_in_dir('java -version', self.root_dir, self.jdk7_env)

      else:
         download_cmd = 'bash {} --installDir {}'.format(jdk_script_name, install_path)
         java_home = os.path.join(install_path, 'openJDK-8u292')
         remove_file(java_home)
         self.run_in_dir(download_cmd, self.root_dir)
         # export java home
         jdk_env = os.environ.copy()
         jdk_env['JAVA_HOME'] = java_home
         jdk_env['PATH'] = '{}/bin:{}'.format(java_home, jdk_env['PATH'])
         jdk_env['CLASSPATH'] = '.:{}/lib/dt.jar:{}/lib/tools.jar'.format(java_home, java_home)
         self.jdk_env = jdk_env
         self.run_in_dir('java -version', self.root_dir, self.jdk_env)

   def compile_base_module(self):
      print_log('Begine compile base module ')
      compile_args = self.opt_mgr.get_compile_args()
      # add base module
      compile_args.append('--all')
      cmd = 'scons {}'.format(' '.join(compile_args))
      self.run_in_dir(cmd, self.root_dir)
      print_log("Finish compile base module")

   def compile_driver(self, db_version):
      self.deploy_java_driver(db_version)
      self.compile_java_driver(db_version)
      self.compile_php_driver()
      self.compile_python_driver()

   def compile_java_driver(self, db_version):
      print_log('Begine compile java driver')
      java_dir = os.path.join(self.root_dir, 'driver/java')
      # reset version
      self.set_pom_version(db_version, java_dir)
      # need to install java driver for next compile
      compile_java_cmd = 'mvn clean install -Dmaven.test.skip=true -Dmaven.javadoc.skip=true'
      self.run_in_dir(compile_java_cmd, java_dir, self.jdk_env)
      print_log('Finish compile java driver')

   def deploy_java_driver(self, db_version):
      print_log('Begine deploy java driver')
      java_dir = os.path.join(self.root_dir, 'driver/java')
      # reset version to snapshot
      snapshot_version = db_version + '-SNAPSHOT'
      self.set_pom_version(snapshot_version, java_dir)
      # donot use -P release, because it need gpg sign
      deploy_java_cmd = 'mvn clean deploy -Dmaven.test.skip=true'
      self.run_in_dir(deploy_java_cmd, java_dir, self.jdk_env)
      print_log('Finish deploy java driver')

   def compile_php_driver(self):
      print_log('Begine compile php driver')
      script_dir = os.path.join(self.root_dir, 'thirdparty/php/linux')
      php_dir = os.path.join(self.root_dir, 'driver/php')
      php_list = ['5.3.3', '5.3.8', '5.3.10', '5.3.15', '5.4.6',
                  '5.5.0', '5.6.0', '7.0.0', '7.1.11']
      if self.debug:
         compile_cmd = 'scons {} -j {}'.format('--dd', self.job)
      else:
         compile_cmd = 'scons -j {}'.format(self.job)
      # chmod script first
      self.run_in_dir('chmod u+x retriConfig.sh', script_dir)
      for php_version in php_list:
         exec_script = './retriConfig.sh {}'.format(php_version)
         self.run_in_dir(exec_script, script_dir)
         # actual compile
         compile_php_cmd = compile_cmd + ' --phpversion={}'.format(php_version)
         self.run_in_dir(compile_php_cmd, php_dir)
      print_log('Finish compile php driver')

   def compile_python_driver(self):
      if OS_TYPE == 'Linux':
         print_log('Begine compile python driver')
         python_dir = os.path.join(self.root_dir, 'driver/python')
         delete_python_driver = 'rm -rf pysequoiadb*.tar.gz build'
         if self.debug:
            compile_python2_driver = 'scons {} -j {}'.format('--dd', self.job)
            compile_python3_driver = 'scons --py3 {} -j {}'.format('--dd', self.job)
         else:
            compile_python2_driver = 'scons -j {}'.format(self.job)
            compile_python3_driver = 'scons --py3 -j {}'.format(self.job)
         self.run_in_dir(delete_python_driver, python_dir)
         self.run_in_dir(compile_python2_driver, python_dir)
         self.run_in_dir(compile_python3_driver, python_dir)
         print_log('Finish compile python driver')

   def compile_connector(self, db_version):
      self.compile_pg_connector()
      self.compile_om_plugin(db_version)
      self.compile_s3(db_version)
      self.compile_spark(db_version)
      self.compile_spark3(db_version)
      self.compile_flink(db_version)

   def compile_pg_connector(self):
      print_log('Begine compile postgresql connector')
      pg_connector_dir = os.path.join(self.root_dir, 'driver/postgresql')
      if self.debug:
         build_pg_connector_cmd = 'make ver=debug'
      else:
         build_pg_connector_cmd = 'make'
      self.run_in_dir(build_pg_connector_cmd + ' clean', pg_connector_dir)
      self.run_in_dir(build_pg_connector_cmd + ' local', pg_connector_dir)
      self.run_in_dir(build_pg_connector_cmd + ' all', pg_connector_dir)
      print_log('Finish compile postgresql connector')

   def compile_om_plugin(self, db_version):
      print_log('Begine compile postgresql plugin')
      om_plugin_dir = os.path.join(self.root_dir, 'tools/om_plugins/sequoiasql/source')
      # reset version
      self.set_pom_version(db_version, om_plugin_dir)
      compile_om_plugin_cmd = 'mvn clean package -Dmaven.test.skip=true'
      self.run_in_dir(compile_om_plugin_cmd, om_plugin_dir, self.jdk_env)
      print_log('Finish compile postgresql plugin')

   def compile_s3(self, db_version):
      print_log('Begine compile sequoias3')
      s3_dir = os.path.join(self.root_dir, 'SequoiaDB/engine/tools/sequoias3')
      compile_s3_cmd = 'mvn clean package -Dmaven.test.skip=true'
      self.set_pom_version(db_version, s3_dir)
      self.run_in_dir(compile_s3_cmd, s3_dir, self.jdk_env)
      s3_dir = os.path.join(s3_dir, 'driver/java')
      self.set_pom_version(db_version, s3_dir)
      self.run_in_dir(compile_s3_cmd, s3_dir, self.jdk_env)
      print_log('Finish compile sequoias3')

   def compile_spark(self, db_version):
      print_log('Begine compile spark connector')
      spark_dir = os.path.join(self.root_dir, 'driver/spark')
      spark_cmd = 'mvn clean package -Dsequoiadb.driver.version={}'.format(db_version)
      self.set_pom_version(db_version, spark_dir)
      self.run_in_dir(spark_cmd, spark_dir, self.jdk_env)
      print_log('Finish compile spark')

   def compile_spark3(self, db_version):
      print_log('Begine compile spark3 connector')
      spark3_dir = os.path.join(self.root_dir, 'driver/spark-3.0')
      spark3_cmd = 'mvn clean package -Dmaven.test.skip=true -Dsequoiadb.driver.version={}'.format(db_version)
      self.set_pom_version(db_version, spark3_dir)
      self.run_in_dir(spark3_cmd, spark3_dir, self.jdk_env)
      print_log('Finish compile spark3')
      
   def compile_flink(self, db_version):
      print_log('Begine compile spark connector')
      flink_dir = os.path.join(self.root_dir, 'driver/flink')
      flink_cmd = 'mvn clean package -Dmaven.test.skip=true -Dsequoiadb.driver.version={}'.format(db_version)
      self.set_pom_version(db_version, flink_dir)
      self.run_in_dir(flink_cmd, flink_dir, self.jdk_env)
      print_log('Finish compile flink')

   def set_pom_version(self, db_version, src_path):
      set_version_cmd = 'mvn versions:set -DnewVersion={}'.format(db_version)
      self.run_in_dir(set_version_cmd, src_path)

   def run_in_dir(self, cmd, dir, env=None):
      print_log('Run command: {} in dir {}'.format(cmd, dir))
      process = Popen(cmd, shell=True, cwd=dir, stdout=PIPE, stderr=PIPE, env=env)
      out, err = process.communicate()
      if 0 == process.returncode:
         print_log(out)
      else:
         print_log(err)
         err_msg = 'Run command {} fail in dir {}'.format(cmd, dir)
         self.err_exit(process.returncode, err_msg)

   def err_exit(self, rs, err_msg ):
      if rs != 0:
         print_log('Return code: {}'.format(rs))
         print_log(err_msg)
         sys.exit(1)

def print_log(log):
   if LOG_FILE:
      file = codecs.open(LOG_FILE, 'a', encoding='utf-8')
      if not isinstance(log, unicode):
         log = unicode(log, 'utf-8')
      file.write(log)
      file.write('\n')
      file.flush()
      file.close()
   else:
      print(log)

def remove_file(src):
   if os.path.exists(src):
      if os.path.isdir(src):
         shutil.rmtree(src)
      else:
         os.remove(src)
############################################
# Decription:
#   Compile source code, include: doc, C#
############################################
import os,sys
import shutil
import argparse
import glob
import tarfile
import time

class OptionsMgr:
   def __init__(self):
      self.__parser = argparse.ArgumentParser()
      compile_group = self.__parser.add_argument_group('compile arguments')
      compile_group.add_argument(
         '-j', '--job', metavar = 'jobNum', type = int, default = 4,
         help = 'Compile thread number, default: 4'
      )

   def parse_args(self):
      self.args = self.__parser.parse_args()
      
   def get_job_num(self):
      return self.args.job

class CompileExModuleMgr:
   def __init__(self, opt_mgr):
      self.root_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir))
      self.cur_dir = os.path.abspath(os.path.dirname(__file__))
      self.package_dir = os.path.join(self.root_dir, 'ex_module')
      self.job = opt_mgr.get_job_num()

   def compile_doc(self):
      build_dir = os.path.join(self.root_dir, 'doc/build')
      out_put_dir = os.path.join(build_dir, 'output')

      self.remove_file(build_dir)
      print('Begine compile doc')
      # -j parameter have no effect in compile doc
      scons_doc = 'scons --doc'
      # compile fail because doc, dont check rs for now
      self.exec_compile(scons_doc, self.root_dir)
      self.copy_file(out_put_dir, '*.pdf')
      print('Finish compile doc')

      # compile chm
      print('Begine compile chm')
      scons_chm = 'scons --chm'
      rs = self.exec_compile(scons_chm, self.root_dir)
      self.err_exit(rs, 'Compile chm fail')
      # copy chm
      self.copy_file(out_put_dir, '*.chm')
      print('Finish compile chm')

      # compile offline website
      print('Begine compile offline')
      scons_offline = 'scons --offline'
      rs = self.exec_compile(scons_offline, self.root_dir)
      self.err_exit(rs, 'Compile offline fail')
      print('Finish compile offline')
      
      offline_dir = os.path.join(out_put_dir, 'SequoiaDB_usermanuals_v*')
      full_file_name = glob.glob(offline_dir)[0]
      file_name = os.path.basename(full_file_name)
      tar = tarfile.open(full_file_name + '.tar.gz', 'w:gz')
      tar.add(full_file_name, arcname = file_name)
      tar.close()
      self.copy_file(out_put_dir, 'SequoiaDB_usermanuals_v*.tar.gz')
      
      # compile website
      print('Begine compile website')
      scons_website = 'scons --website'
      rs = self.exec_compile(scons_website, self.root_dir)
      self.err_exit(rs, 'Compile website fail')
      print('Finish compile website')

   def exec_compile(self, cmd, exec_dir):
      self.set_env_dir = os.path.join(self.root_dir, 'script/SetEnv.cmd')
      compile_cmd = '\""cmd /E:ON /V:ON /T:0E /K {} && {} & EXIT "\"'.format(self.set_env_dir, cmd)
      os.chdir(exec_dir)
      rs = os.system(compile_cmd)
      os.chdir(self.cur_dir)
      # wait doc program exit
      time.sleep(30)
      return rs

   # for the copy file, it will copy to the package dir
   def copy_file(self, src_path, file_name):
      files = glob.glob(os.path.join(src_path, file_name))
      for file in files:
         shutil.copy(file, self.package_dir)

   # we need to package file to scp
   def package_file(self):
      tar_file = os.path.join(self.root_dir, 'ex_module.tar.gz')
      tar = tarfile.open(tar_file, 'w:gz')
      tar.add(os.path.join(self.root_dir, 'ex_module'), arcname = 'ex_module')
      tar.close()
      
   def create_package_dir(self):
      # prepare file
      self.remove_file(self.package_dir)
      os.makedirs(self.package_dir)

   def remove_file(self, src_path):
      if os.path.exists(src_path):
         if os.path.isdir(src_path):
            shutil.rmtree(src_path)
         else:
            os.remove(src_path)
   
   def err_exit(self, rs, err_msg ):
      if rs != 0:
         print( err_msg )
         sys.exit(1)

def main():
   opt_mgr = OptionsMgr()
   opt_mgr.parse_args()
   compile_mgr = CompileExModuleMgr(opt_mgr)
   compile_mgr.create_package_dir()
   compile_mgr.compile_doc()
   compile_mgr.package_file()
   
if __name__ == "__main__":
   sys.exit(main())
############################################
#decription:
#   compile source code, include:
#   engineer, driver, connector
############################################

import os,sys
import getopt
import commands
import platform


def help_info():
   print ('usage: python build.py')
   print ('')
   print ('')
   print ('   --dd                     debug build no optimization')
   print ('   --release                release build')
   print ('   --enterprise             edition type: enterprise, default: community')
   print ('   -h, --help               show this help info')
   print ('')
   print ('')
   print ('Examples:')
   print ('   python build.py --dd')
   print ('')
   print ('')

def run_in_dir( cmd, dir ):
   os.chdir( dir )
   rs = os.system( cmd )
   os.chdir( cur_dir )
   return rs

def err_exit( rs, err_msg ):
   if rs != 0:
      print( err_msg )
      sys.exit(1)

################### begin #################
scrpt_path = sys.path[0]
os_type = platform.system()
cur_dir = os.getcwd()
code_path = scrpt_path
build_type = ''
edition_type = ''

short_args = 'h'
long_args = ['help', 'dd', 'release', 'enterprise']
try:
   opts, args = getopt.getopt(sys.argv[1:], short_args, long_args )
except getopt.GetoptError:
   help_info()
   sys.exit(1)

for opt, arg in opts:
   if opt in ('-h', '--help'):
      help_info()
      sys.exit(0)
   elif opt == '--dd':
      build_type = opt
   elif opt == '--release':
      build_type = ''
   elif opt == '--enterprise':
      edition_type = opt   
   else:
      help_info()
      sys.exit(1)

build_cmd_pre = 'scons' + ' ' + build_type 

#compile engine
build_all_cmd = build_cmd_pre  + ' ' + edition_type + ' ' + '--all -j 4'
rs = run_in_dir( build_all_cmd, code_path )
err_exit( rs, 'Failed to compile engine!')

#compile java driver
java_code_path = code_path + '/driver/java'
build_java_cmd = build_cmd_pre
rs = run_in_dir( build_java_cmd, java_code_path )
err_exit( rs, 'Failed to compile Java driver!' )

#compile fdw 
fdw_code_path = code_path + '/driver/postgresql'
rs = run_in_dir( 'make clean', fdw_code_path )
err_exit( rs, 'Failed to compile fdw in make clean')
rs = run_in_dir( 'make local', fdw_code_path )
err_exit( rs, 'Failed to compile fdw in make local')
rs = run_in_dir( 'make all',   fdw_code_path )
err_exit( rs, 'Failed to compile fdw in make all')

#compile php driver
php_code_path = code_path + '/driver/php5'
build_php_cmd_common = build_cmd_pre + ' --phpversion='
php_ver_file_path = ''
if os_type == 'Windows' or os_type == 'Microsoft':
   php_ver_file_path = php_code_path + '/php_ver_win.list'
elif os_type == 'Linux':
   php_ver_file_path = php_code_path + '/php_ver_linux.list'
else:
   err_exit( 1, 'The platform is not supported!' )
php_ver_file_obj = open( php_ver_file_path )
try:
   while 1:
      line = php_ver_file_obj.readline()
      if not line:
         break
      build_php_cmd = build_php_cmd_common + line      
      rs = run_in_dir( build_php_cmd, php_code_path )
      err_exit( rs, 'Failed to compile PHP driver!' )
finally:
   php_ver_file_obj.close()
   
#compile python
build_python_cmd = build_cmd_pre
python_code_path = code_path + '/driver/python'
rs = run_in_dir( build_python_cmd, python_code_path )
err_exit( rs, 'Failed to compile python driver!')

#compile C# driver
if os_type == 'Windows' or os_type == 'Microsoft':
   c_sharp_code_path = code_path + '/driver/C#.Net'
   build_c_sharp_cmd = build_cmd_pre
   rs = run_in_dir( build_c_sharp_cmd, c_sharp_code_path )
   err_exit( rs, 'Failed to compile C# driver!' )

#compile hive 
hive_code_path = code_path + '/driver/hadoop/hive'
rs = run_in_dir( 'ant', hive_code_path )
err_exit( rs, 'Failed to compile hive!')

#compile hadoop connector 
hdcn_code_path = code_path + '/driver/hadoop/hadoop-connector'
rs = run_in_dir( 'ant -Dhadoop.version=1.2', hdcn_code_path )
err_exit( rs, 'Failed to compile hadoop connector 1.2!')
rs = run_in_dir( 'ant -Dhadoop.version=2.2', hdcn_code_path )
err_exit( rs, 'Failed to compile hadoop connector 2.2!')

#compile spark 
#TODO: zichuan has new compile 
   
print( 'Compile source code completed!' )

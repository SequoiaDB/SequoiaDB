############################################
#decription:
#   install rpm package, steps:
#   1.compile source code, call build.py  
#   2.copy compile output, call cppkgfile.sh
#   3.generate rpm package, call pkgrpm.sh
############################################

import os,sys
import getopt
import commands
import platform

def help_info():
   print ('usage: python package.py [OPTION]')
   print ('')
   print ('')
   print ('   -t                       type of package: rpm')
   print ('   -s                       path of source files. Collect from source')
   print ('                            code directory if not specified.')
   print ('   --dd                     debug build no optimization')
   print ('   --release                release build')
   print ('   --enterprise             edition type: enterprise, default: community')
   print ('   --rmsource               remove souce files when done')
   print ('   --nobuild                do not compile. Effective if "-s" is specified')
   print ('   -h, --help               show this help info')
   print ('')
   print ('')
   print ('Examples:')
   print ('   python package.py -t rpm -s ./package/source')
   print ('')
   print ('')

def check_para():
   if pkg_type == 'rpm':
      rpmchk,outputtmp = commands.getstatusoutput('rpmbuild --version')
      if rpmchk != 0 :
         print( 'ERROR: rpm-build is not installed!' )
         sys.exit(1)
   else:
      help_info()
      sys.exit(1)

################ begin ##################
pkg_type = "rpm"
build_type = "release"
edition_type = 'community' 
src_path=""
cur_dir = os.getcwd()
scrpt_path = sys.path[0]
work_dir = scrpt_path + '/../package'
code_path = scrpt_path + '/..'
rmsrc = False
need_build = True
short_args = 'ht:w:s:b:'
long_args = ['help', 'source-file-path=', 'dd', 'release', 'rmsource', 'nobuild', 'enterprise']
try:
   opts, args = getopt.getopt(sys.argv[1:], short_args, long_args )
except getopt.GetoptError:
   help_info()
   sys.exit(1)

for opt, arg in opts:
   if opt in ('-h', '--help'):
      help_info()
      sys.exit(0)
   elif opt == '-t':
      pkg_type = arg
   elif opt == '-s':
      src_path = arg
   elif opt == '--enterprise':
      edition_type = 'enterprise'       
   elif opt == '--dd':
      build_type = 'debug'
   elif opt == '--release':     
      build_type = 'release'  
   elif opt == '--rmsource':
      rmsrc = True
   elif opt == '--nobuild':
      need_build = False
   else:
      help_info()
      sys.exit(1)

check_para()

rs = 0
if src_path == "":
   os_type = platform.system()
   if os_type == 'Windows' or os_type == 'Microsoft':
      print( 'TODO: prepare the source files in windows!' )
      sys.exit(1)
      
   # 1 compile source code  
   if need_build:
      print( 'compile the source code...' )
      build_scrpt_path = code_path + '/build.py'
      build_cmd_pre = 'python ' + build_scrpt_path
      build_type_tmp = ''
      edition_type_tmp = ''
      if build_type == 'debug':
         build_type_tmp = ' --dd'
      if edition_type == 'enterprise':
         edition_type_tmp = ' --enterprise'   
      build_cmd = build_cmd_pre + build_type_tmp + edition_type_tmp      
      rs = os.system( build_cmd )
      if rs != 0:
         print( 'Error: Failed to compile the source code!' )
         sys.exit( rs )   
   
   # 2 move compile output, ready to pack
   src_path = work_dir + '/tmp/sequoiadb'
   print( 'prepare the source files...' )
   cp_files_cmd = scrpt_path + '/cppkgfiles.sh ' + src_path + ' ' + build_type  
   rs = os.system( cp_files_cmd )
   if rs != 0:
      print( 'ERROR: Failed to prepare the source files!' )
      sys.exit( rs )   
   
# 3 generate rpm package
if pkg_type == "rpm":
   print( 'generate rpm-package...' )
   str_tmp = [ scrpt_path, '/pkgrpm.sh ', src_path, ' ', edition_type ]
   pkg_rpm_cmd = ''.join( str_tmp )
   rs = os.system( pkg_rpm_cmd )
   if rs != 0:
       print( 'ERROR: Failed to generate rpm-package!' )
       sys.exit( rs )

if rmsrc == True:
   print( 'remove source files...' )
   os.system( 'rm -rf src_path' )

print( 'completed!' )

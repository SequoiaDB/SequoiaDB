
import platform
import os
import sys
from os.path import join

mode = "doc"

if len(sys.argv) > 0 and len(sys.argv[1]) > 2:
   mode = sys.argv[1][2:]

root_dir = os.path.split( os.path.realpath( sys.argv[0] ) )[0]

def GuessOS():
    id = platform.system()
    if id == 'Linux':
        return 'linux'
    elif id == 'Windows' or id == 'Microsoft':
        return 'win32'
    elif id == 'AIX':
        return 'aix'
    else:
        return None

errno = 0
os.chdir( root_dir )
if GuessOS() == 'win32':
    php_file = join(root_dir, '..\\tools\\server\\php_win\\php.exe')
    script_file = join(root_dir, "tools\\controller\\main.php")
    execCmd = php_file + ' ' + script_file + ' -m ' + mode
    os.environ['PATH'] = os.environ['PATH'] + ';' + join(root_dir, 'tools\\wordConvertor\\libs' )
    errno = os.system( execCmd )
else:
    php_file = join(root_dir, 'tools/html2mysql/php/bin/php')
    script_file = join(root_dir, 'tools/controller/main.php')
    execCmd = php_file + ' ' + script_file + ' -m ' + mode
    print( os.environ.get('LD_LIBRARY_PATH') )
    if os.environ.get('LD_LIBRARY_PATH') is None:
       os.environ['LD_LIBRARY_PATH'] = join(root_dir, 'tools/html2mysql/php/libxml2/lib') + ':' + join(root_dir, 'tools/html2mysql/php/lib')
    else:
       os.environ['LD_LIBRARY_PATH'] = os.environ.get('LD_LIBRARY_PATH') + ':' + join(root_dir, 'tools/html2mysql/php/libxml2/lib') + ':' + join(root_dir, 'tools/html2mysql/php/lib')
    if os.path.exists( '/usr/lib64/libXrender.so.1' ) == False:
       os.environ['LD_LIBRARY_PATH'] += ':' + join(root_dir, 'tools/pdfConvertor/tools/linux64/wkhtmltox/lib')
    print( "aaa " + os.environ['LD_LIBRARY_PATH'] + "\n\n" )
    os.environ['PATH'] = os.environ['PATH'] + ':' + join(root_dir, 'tools/wordConvertor/libs' )
    errno = os.system( execCmd )

if errno != 0:
   os._exit( 1 )



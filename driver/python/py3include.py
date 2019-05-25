# find python3 C API include directory
import os
import platform
import sys


if sys.version_info[0] != 3:
    raise Exception("not python3")

python_include = ""
python_lib = ""

_os = platform.system()
if _os == 'Linux':
    ver = sys.version_info[0]
    sub = sys.version_info[1]
    python_include = sys.prefix + '/include/' + ("python%d.%d" % (ver, sub))
    if not os.path.exists(python_include):
        python_include += "m"
        if not os.path.exists(python_include):
            raise Exception("can't find python3 include directory")
    python_lib = sys.prefix + '/lib/' + ("python%d.%d" % (ver, sub))
elif _os == 'Windows':
    python_include = sys.prefix + "/include/"
    python_lib = sys.prefix + '/libs/'

print(python_include)
print(python_lib)

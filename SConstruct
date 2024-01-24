# -*- mode: python; -*-
# build file for SequoiaDB
# this requires scons
# you can get from http://www.scons.org
# then just type scons

# some common tasks
#   build 64-bit mac and pushing to s3
#      scons --64 s3dist

# This file, SConstruct, configures the build environment, and then delegates to
# several, subordinate SConscript files, which describe specific build rules.

EnsureSConsVersion( 1, 1, 0 )

import platform
import os
import sys
import imp
import types
import re
import shutil
import subprocess
import urllib
import urllib2
import stat
from os.path import join, dirname, abspath
import libdeps

import scons_compiledb

root_dir = dirname(File('SConstruct').rfile().abspath)
db_dir = join(root_dir,'SequoiaDB')
engine_dir = join(db_dir,'engine')
thirdparty_dir = join(root_dir, 'thirdparty')
boost_dir = join(thirdparty_dir, 'boost')
boost_lib_dir = join(boost_dir, 'lib')
parser_dir = join(thirdparty_dir, 'parser' )
sm_dir = join(parser_dir, 'sm')
js_dir = join(sm_dir, 'js')
sm_lib_dir = join(js_dir, 'lib')
intel_decimal_dir = join(thirdparty_dir, 'intelDecimal')
intel_decimal_lib_dir = join(intel_decimal_dir, 'lib')
pcre_dir = join(engine_dir,'pcre')
ssh2_dir = join(engine_dir,'ssh2')
crypto_dir = join(thirdparty_dir, 'crypto')
ssl_dir = join(crypto_dir, 'openssl-1.1.1o')
ssl_lib_dir = join(ssl_dir, 'lib')
mdocml_dir = join(thirdparty_dir, 'mdocml' )
mdocml_lib_dir = join(mdocml_dir, 'lib')
lz4_dir = join(thirdparty_dir, 'lz4')
lz4_lib_dir = join(lz4_dir, 'lib')
zlib_dir = join(thirdparty_dir, 'zlib')
zlib_lib_dir = join(zlib_dir, 'lib')
snappy_dir = join(thirdparty_dir, 'snappy')
snappy_lib_dir = join(snappy_dir, 'lib')
gtest_dir = join(engine_dir,'gtest')
ncursesinclude_dir = join(engine_dir, 'ncurses/include')
driver_dir = join(db_dir,'driver')
java_dir = join(root_dir,'java')
fuse_dir = join(thirdparty_dir, 'fuse')
fuse_lib_dir = join(fuse_dir, 'lib')
zstd_dir = join(thirdparty_dir, 'zstd')
zstd_lib_dir = join(zstd_dir, 'lib')
wiredtiger_dir = join(thirdparty_dir, 'wiredtiger')
# --- options ----

options = {}

options_topass = {}

def mergeStaticLibrary(target, aix, *sources):
    if not sys.platform.startswith('linux') and not sys.platform.startswith('aix'):
        raise Exception('mergeStaticLibrary currently only support linux or aix')
    if not os.path.isabs(target):
        raise Exception('target must be a absolute path: ' + target)
    path = os.path.dirname(target)
    file = os.path.basename(target)
    if not os.path.exists(path):
        raise Exception('target path not exists: ' + path)
    if not file.endswith('.a'):
        raise Exception('target not ends with ".a": ' + file)
    if os.path.exists(target):
        os.remove(target)
    subdir = path + '/' + file[0:file.index(".a")] + '.objs'
    if os.path.exists(subdir):
        shutil.rmtree(subdir)
    os.mkdir(subdir)
    #print("current directory is " + os.getcwd())
    print("create objs directory: " + subdir )
    cmd = ""
    for s in sources:
        if not os.path.isabs(s):
            raise Exception('source must be a absolute path: ' + s)
        if not os.path.exists(s):
            raise Exception('source not exists: ' + s)
        print("extract objs from " + s)
        if aix:
            cmd = "ar -X32_64 x " + s
        else:
            cmd = "ar x " + s
        print(cmd)
        subprocess.check_call(cmd, shell=True)
        if aix:
            cmd = "mv `ar -X32_64 t " + s + "` " + subdir
        else:
            cmd = "mv `ar t " + s + "` " + subdir
        print(cmd)
        subprocess.check_call(cmd, shell=True)
    if aix:
        cmd = "ar -X32_64 cr " + target + " " + subdir + "/*.o"
    else:
        cmd = "ar cr " + target + " " + subdir + "/*.o"
    print(cmd)
    subprocess.check_call(cmd, shell=True)
    cmd = "ranlib " + target
    print(cmd)
    subprocess.check_call(cmd, shell=True)
    shutil.rmtree(subdir)
    print("remove objs directory: " + subdir)

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


def GuessArch():
   id = platform.machine()
   id = id.lower()
   if (not id) or (not re.match('(x|i[3-6])86$', id) is None):
      return 'ia32'
   elif id == 'i86pc':
      return 'ia32'
   elif id == 'x86_64':
      return 'ia64'
   elif id == 'amd64':
      return 'ia64'
   elif id == 'aarch64':
      return 'arm64'
   elif id == 'ppc64':
      return 'ppc64'
   elif id == 'ppc64le':
      return 'ppc64le'
   elif id == 'sw_64':
      return 'alpha64'
   else:
      return None

# guess the operating system and architecture
guess_os = GuessOS()
guess_arch = GuessArch()

# helper function, add options
# name: name of the parameter
# nargs: number of args for the parameter
# contibutesToVariantDir: whether the param is part of variant dir
def add_option( name, help , nargs , contibutesToVariantDir , dest=None ):

    if dest is None:
        dest = name

    AddOption( "--" + name ,
               dest=dest,
               type="string",
               nargs=nargs,
               action="store",
               help=help )

    options[name] = { "help" : help ,
                      "nargs" : nargs ,
                      "contibutesToVariantDir" : contibutesToVariantDir ,
                      "dest" : dest }

def get_option( name ):
    return GetOption( name )

def _has_option( name ):
    x = get_option( name )
    if x is None:
        return False

    if x == False:
        return False

    if x == "":
        return False

    return True

def has_option( name ):
    x = _has_option(name)

    if name not in options_topass:
        # if someone already set this, don't overwrite
        options_topass[name] = x

    return x


def get_variant_dir():

    a = []

    for name in options:
        o = options[name]
        if not has_option( o["dest"] ):
            continue
        # let's skip the param if it's not part of variant dir
        if not o["contibutesToVariantDir"]:
            continue

        if o["nargs"] == 0:
            a.append( name )
        else:
            x = get_option( name )
            x = re.sub( "[,\\\\/]" , "_" , x )
            a.append( name + "_" + x )

    s = "#build/${PYSYSPLATFORM}/"

    if len(a) > 0:
        a.sort()
        s += "/".join( a ) + "/"
    else:
        s += "normal/"
    return s

def get_platform_dir():
    if "linux" == guess_os:
        if "ia32" == guess_arch:
            return "linux32"
        elif "ia64" == guess_arch:
            return "linux64"
        elif "arm64" == guess_arch:
            return "armlinux64"
        elif "ppc64" == guess_arch:
            return "ppclinux64"
        elif "ppc64le" == guess_arch:
            return "ppclelinux64"
        elif "alpha64" == guess_arch:
            return "alphalinux64"
    elif "win32" == guess_os:
        if "ia64" == guess_arch:
            return "win64"
        else:
            return "win32"
    elif "aix" == guess_os:
        return "aix64"

# build options
add_option( "all", "build engine/tools/testcases/shell/client/fmp/fap", 0, False)
add_option( "engine", "build engine", 0, False)
add_option( "tool", "build tools", 0, False)
add_option( "testcase", "build testcases", 0, False)
add_option( "shell", "build shell", 0, False)
add_option( "client", "build C/C++ clients", 0, False)
add_option( "fmp", "build fmp", 0, False)
add_option( "doc", "build document(pdf, word)", 0, False)
add_option( "website", "build web site document", 0, False)
add_option( "chm", "build chm document", 0, False)
add_option( "offline", "build offline html document", 0, False)
add_option( "doxygen", "build doxygen document", 0, False)
add_option( "noautogen", "do not run autogen", 0, False)
add_option( "nolinkssl", "do not link openssl when building C/C++ clients(only for linux).", 0, False )

# language could be en or cn
add_option( "language" , "description language" , 1 , False )

# linking options
add_option( "release" , "release build" , 0 , True )

# dev options
add_option( "dd", "debug build no optimization" , 0 , True , "debugBuild" )
add_option( "noscreenout", "do not send anything to screen", 0, True )

#fap options
add_option( "fap", "foreign access protocol", 0, False )

#enterprise options
add_option( "enterprise", "build enterprise sequoiadb ( with SSL )", 0, False )

#gprof option
add_option("gprof", "enable gprofile for sequoiadb", 0, False)

#aix xlc
add_option("xlc", "use xlc in AIX", 0, False)

#coverage option
add_option( "cov" , "generate coverage information" , 0, False )

#compiledb
add_option( "compiledb", "generate compile_commands.json", 0, False )

#suppressWarning
add_option( "suppressWarning", "suppress warning", 0, False )

# don't run configure if user calls --help
if GetOption('help'):
    Return()

# --- environment setup ---
variantDir = get_variant_dir()
clientCppVariantDir = variantDir + "clientcpp"
clientCVariantDir = variantDir + "clientc"
shellVariantDir = variantDir + "shell"
toolVariantDir = variantDir + "tool"
fmpVariantDir = variantDir + "fmp"
driverDir = variantDir + "driver"
fapVariantDir = variantDir + "fap"
wtVariantDir = variantDir + "wiredtiger"

def printLocalInfo():
   import sys, SCons
   print( "scons version: " + SCons.__version__ )
   print( "python version: " + " ".join( [ `i` for i in sys.version_info ] ) )

printLocalInfo()

boostLibs = [ "thread" , "filesystem", "program_options", "system", "chrono" ]

nix = False
linux = False
linux64 = False
windows = False
aix = False
xlc = False

if guess_os == "aix":
    xlc = has_option("xlc")

release = True
debugBuild = False
suppressWarning = False

release = has_option( "release" )

# get whether we are using debug build
debugBuild = has_option( "debugBuild" )
suppressWarning = has_option( "suppressWarning" )

# if neither release/debugBuild specified, by default using release
# if both release/debugBuild specified, by defaul use debugBuild
if not release and not debugBuild:
   release = True
   debugBuild = False
elif release and debugBuild:
   release = False
   debugBuild = True

# Debug and release libraries are placed in seperated directories.
build_dir = None
if release:
    build_dir = "release"
else:
    build_dir = "debug"

cov = False

cov = has_option( "cov" )

# do not generate coverage info when release
if not debugBuild and cov:
   cov = False

env = Environment( BUILD_DIR=variantDir,
                   tools=["default", "gch", "mergelib", "textfile" ],
                   PYSYSPLATFORM=os.sys.platform,
                   )
env.Decider( "MD5-timestamp" )

needCompileDb = False
needCompileDb = has_option( "compiledb" )
if needCompileDb:
    compileDBConfig = scons_compiledb.Config( db = join( "#build", "compile_commands.json" ) )
    scons_compiledb.enable( env, compileDBConfig )
    env.CompileDb()

if guess_os == "linux":
   env.Append( CXXFLAGS=" -std=c++11 " )
   # Ignore warnings caused by the C++11 standard in debug version,
   # 'template<class> class std::auto_ptr' is deprecated.
   if debugBuild:
      env.Append( CXXFLAGS=" -Wno-deprecated-declarations -Wno-parentheses " )
   if suppressWarning:
      env.Append( CXXFLAGS=" -w " )
      env.Append( CFLAGS=" -w " )

libdeps.setup_environment( env )

if env['PYSYSPLATFORM'] == 'linux3':
   env['PYSYSPLATFORM'] = 'linux2'

if os.sys.platform == 'win32':
   env['OS_FAMILY'] = 'win'
else:
   env['OS_FAMILY'] = 'posix'

if env['PYSYSPLATFORM'] == 'linux2':
   env['LINK_LIBGROUP_START'] = '-Wl,--start-group'
   env['LINK_LIBGROUP_END'] = '-Wl,--end-group'
   env['RELOBJ_LIBDEPS_START'] = '--whole-archive'
   env['RELOBJ_LIBDEPS_END'] = '--no-whole-archive'
   env['RELOBJ_LIBDEPS_ITEM'] = ''

env["LIBPATH"] = []

if has_option( "noscreenout" ):
    env.Append( CPPDEFINES=[ "_NOSCREENOUT" ] )

hasEngine = has_option( "engine" )
hasClient = has_option( "client" )
hasTestcase = has_option( "testcase" )
hasTool = has_option( "tool" )
hasShell = has_option( "shell" )
hasFmp = has_option("fmp")
hasAll = has_option( "all" )
hasDoc = has_option( "doc" )
hasWebSite = has_option( "website" )
hasChm = has_option( "chm" )
hasOffline = has_option( "offline" )
hasDoxygen = has_option( "doxygen" )
hasNoLinkSSL = has_option("nolinkssl")

hasFap = False
if guess_os == "win32":
    hasFap = False
else:
    hasFap = has_option("fap")
hasEnterprise = has_option("enterprise")
hasGProf = has_option("gprof")
hasSSL = False

# build enterprise edition
if hasEnterprise:
   hasSSL = True
   env.Append( CPPDEFINES=[ "SDB_ENTERPRISE" ] )

# if everything are set, let's set everything to true
if hasAll:
   hasEngine = True
   hasClient = True
   hasTestcase = True
   hasTool = True
   hasShell = True
   hasFmp = True
   if guess_os == "win32":
      hasFap = False
   else:
      hasFap = True
# if nothing specified, let's use engine+client+shell by default
elif not ( hasEngine or hasClient or hasTestcase or hasTool or hasShell or
           hasFmp or hasFap or hasDoc or hasWebSite or hasChm or hasOffline or
           hasDoxygen ):
   hasEngine = True
   hasClient = True
   hasShell = True
   hasTool = True
   hasFmp = True
elif ( hasTestcase and not hasEngine ):
   hasEngine = True

boostCompiler = ""
boostVersion = ""

usesm = False
usemdocml = False
usefuse = False
if guess_os == 'linux' or guess_os == 'win32':
   usesm = True
if guess_os == 'linux' or guess_os == 'win32':
    if guess_arch == "ia64" or guess_arch == "arm64" or guess_arch == "ppc64le" or guess_arch == "alpha64":
        usemdocml = True
if guess_os == 'linux':
    if guess_arch == "ia64" or guess_arch == "arm64" or guess_arch == "ppc64le" or guess_arch == "alpha64":
        usefuse = True

extraLibPlaces = []

env['EXTRACPPPATH'] = []
env['EXTRALIBPATH'] = []

class InstallSetup:
    binaries = False
    clientSrc = False
    headers = False
    bannerFiles = tuple()
    headerRoot = "include"

    def __init__(self):
        self.default()

    def default(self):
        self.binaries = True
        self.libraries = False
        self.clientSrc = False
        self.headers = False
        self.bannerFiles = tuple()
        self.headerRoot = "include"
        self.clientTestsDir = None

    def justClient(self):
        self.binaries = False
        self.libraries = False
        self.clientSrc = True
        self.headers = True
        self.bannerFiles = [ "#distsrc/client/LICENSE.txt",
                             "#distsrc/client/SConstruct" ]
        self.headerRoot = ""

installSetup = InstallSetup()

# ---- other build setup -----

if "uname" in dir(os):
    processor = os.uname()[4]
else:
    processor = "i386"

env['PROCESSOR_ARCHITECTURE'] = processor

DEFAULT_INSTALL_DIR = "/opt/sequoiadb"
installDir = DEFAULT_INSTALL_DIR
nixLibPrefix = "lib"

def findVersion( root , choices ):
    if not isinstance(root, list):
        root = [root]
    for r in root:
        for c in choices:
            if ( os.path.exists( r + c ) ):
                return r + c
    raise RuntimeError("can't find a version of [" + repr(root) + "] choices: "
                       + repr(choices))

env.Append(
CPPPATH=[join(engine_dir,'include'),join(engine_dir,'client'),
         join(ssl_dir,'include'),join(lz4_dir,'include'),join(zlib_dir,'./'),
         join(snappy_dir,'include'),join(zstd_dir,'lib'),join(gtest_dir,'include'),
         pcre_dir, boost_dir, ssh2_dir] )

env.Append( CPPDEFINES=["__STDC_LIMIT_MACROS", "HAVE_CONFIG_H", "BOOST_THREAD_HAS_CONDATTR_SET_CLOCK_MONOTONIC"] )
env.Append( CPPDEFINES=[ "SDB_DLL_BUILD" ] )

# Add third party libraries.
if guess_os is not None:
    platform_dir = get_platform_dir()
    boost_lib_dir = join(boost_lib_dir, platform_dir, build_dir)
    ssl_lib_dir = join(ssl_lib_dir, platform_dir, build_dir)
    zlib_lib_dir = join(zlib_lib_dir, platform_dir, build_dir)
    lz4_lib_dir = join(lz4_lib_dir, platform_dir, build_dir)
    snappy_lib_dir = join(snappy_lib_dir, platform_dir, build_dir)
    zstd_lib_dir = join(zstd_lib_dir, platform_dir, build_dir)
    intel_decimal_lib_dir = join(intel_decimal_lib_dir, platform_dir, build_dir)
    env.Append(EXTRALIBPATH=[boost_lib_dir, ssl_lib_dir, zlib_lib_dir,
                             lz4_lib_dir, snappy_lib_dir, zstd_lib_dir,
                             intel_decimal_lib_dir])
    # use project-related spidermonkey library
    if usesm:
        env.Append(CPPPATH=join(sm_lib_dir, platform_dir, 'include'))
        sm_lib_dir = join(sm_lib_dir, platform_dir, build_dir)
        env.Append(EXTRALIBPATH=[sm_lib_dir])
    if usemdocml:
        # for engine does not need to use mdocml, so we are not going to add
        # its lib_dir to env
        mdocml_lib_dir = join(mdocml_lib_dir, platform_dir, build_dir)
    if usefuse:
        # for engine does not need to use fuse, so we are not going to add
        # its lib_dir to env
        fuse_lib_dir = join(fuse_lib_dir, platform_dir, '')

# specify dependent libraries for javascript engine and boost
if guess_os == "linux":
    linux = True

    # -lm
    env.Append( LIBS=['m'] )
    # -ldl
    env.Append( LIBS=['dl'] )
    # -lpthread
    env.Append( LIBS=["pthread"] )
    # GNU
    env.Append( CPPDEFINES=[ "_GNU_SOURCE" ] )
    # in case for 32 bit linux or compiling 32 bit in 64 env
    if guess_arch == "ia32":
        linux64 = False
        nixLibPrefix = "lib"
        env.Append( EXTRALIBPATH="/lib" )
    # 64 bit linux
    elif guess_arch == "ia64":
        linux64 = True
        nixLibPrefix = "lib64"
        env.Append( EXTRALIBPATH="/lib64" )
    elif guess_arch == "arm64":
        linux64 = True
        nixLibPrefix = "lib64"
        env.Append( EXTRALIBPATH="/lib64" )
    # power pc linux
    elif guess_arch == "ppc64":
        linux64 = False
        nixLibPrefix = "lib64"
        # use big endian
        env.Append( CPPDEFINES=[ "SDB_BIG_ENDIAN" ] )
        env.Append( EXTRALIBPATH="/lib64" )
    elif guess_arch == "ppc64le":
        linux64 = True
        nixLibPrefix = "lib64"
        env.Append( CPPDEFINES=[ "SDB_LITTLE_ENDIAN" ] )
        env.Append( EXTRALIBPATH="/lib64" )
    elif guess_arch == "alpha64":
        linux64 = True
        nixLibPrefix = "lib64"
        env.Append( CPPDEFINES=[ "SDB_LITTLE_ENDIAN" ] )
        env.Append( EXTRALIBPATH="/lib64" )
        env.Append( CPPFLAGS=" -mieee " )

    # Building for mysqld without linking openssl in c/c++ client.
    # Or there will be two same openssl symbol existing in one mysqld program
    # in case of mysqld building openssl in.
    if hasNoLinkSSL == False:
        env.Append(LIBS=['ssl', 'crypto'])

    ssllib_file = join(ssl_lib_dir, 'libcrypto.a')
    ssllib_file1 = join(ssl_lib_dir, 'libssl.a')

    # spider monkey
    if usesm:
        smlib_file = join(sm_lib_dir, 'libmozjs185.so')
        env.Append( CPPDEFINES=[ "XP_UNIX" ] )
        env.Append( LIBS=['js_static'] )
    # fuse
    if usefuse:
        fuse_lib = join(fuse_lib_dir, 'libfuse.a')
    # mdocml
    if usemdocml:
        mdocml_lib = join(mdocml_lib_dir, 'libmdocml.a')
    # lz4, zlib and snappy
    zlib_lib = join(zlib_lib_dir, 'libzlib.a')
    lz4_lib = join(lz4_lib_dir, 'liblz4.a')
    snappy_lib = join(snappy_lib_dir, 'libsnappy.a')
    # zstd
    zstd_lib = join(zstd_lib_dir, 'libzstd.a')

    nix = True

elif guess_os == "win32":
    # when building windows
    windows = True
    # check VC compiler
    for pathdir in env['ENV']['PATH'].split(os.pathsep):
        if os.path.exists(os.path.join(pathdir, 'cl.exe')):
            print( "found visual studio at " + pathdir )
            break
        else:
            #use current environment
            env['ENV'] = dict(os.environ)

    if platform.version().split('.')[0] >= '6':
        env.Append( CPPDEFINES=[ "USE_SRW" ] )

    if usesm:
        smlib_file = join(sm_lib_dir, 'mozjs185-1.0.dll')
        env.Append( CPPDEFINES=[ "XP_WIN" ] )
        env.Append( LIBS=['mozjs185-1.0'] )
        env.Append( CPPDEFINES=["JS_HAVE_STDINT_H"] )
    if usemdocml:
        mdocml_lib = join(mdocml_lib_dir, 'libmdocml.lib')

    # SSL
    # env.Append( LIBS=['ssleay32', 'libeay32', 'liblz4', 'libzlib', 'libsnappy'])
    env.Append( LIBS=['libcrypto', 'libssl'])
    ssllib_file = join(ssl_lib_dir, 'libcrypto.lib')
    ssllib_file1 = join(ssl_lib_dir, 'libssl.lib')
    zlib_lib = join(zlib_lib_dir, 'libzlib.lib')
    lz4_lib = join(lz4_lib_dir, 'liblz4.lib')
    snappy_lib = join(snappy_lib_dir, 'libsnappy.lib')
    zstd_lib = join(zstd_lib_dir, 'libzstd.lib')

    # UNICODE
    env.Append( CPPDEFINES=[ "_UNICODE" ] )
    env.Append( CPPDEFINES=[ "UNICODE" ] )
    # warning macro
    env.Append( CPPDEFINES=[ "_SCL_SECURE_NO_WARNINGS" ] )
    # find windows SDK
    winSDKHome = findVersion( [ "C:/Program Files/Microsoft SDKs/Windows/", "C:/Program Files (x86)/Microsoft SDKs/Windows/" ] ,
                              [ "v7.1", "v7.0A", "v7.0", "v6.1", "v6.0a", "v6.0" ] )
    print( "Windows SDK Root '" + winSDKHome + "'" )

    env.Append( EXTRACPPPATH=[ winSDKHome + "/Include" ] )

    env.Append( EXTRACPPPATH=[ "C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC/include" ] )

    env.Append( CPPFLAGS=" /EHsc /W3 " )
    # disable warning
    env.Append( CPPFLAGS=" /wd4355 /wd4800 /wd4267 /wd4244 /wd4200 /wd4251 /wd4275 /wd4273 " )

    env.Append( CPPDEFINES=["_CONSOLE","_CRT_SECURE_NO_WARNINGS","PSAPI_VERSION=1","_CRT_RAND_S" ] )

    if release:
        env.Append( CPPDEFINES=[ "NDEBUG" ] )
        env.Append( CPPFLAGS= " /O2 /Gy " )
        env.Append( CPPFLAGS= " /Zi /errorReport:none " )
        env.Append( CPPFLAGS= " /GL " )
        env.Append( LINKFLAGS=" /LTCG " )
        env.Append( LINKFLAGS=" /DEBUG " )
    else:
        env.Append( CPPFLAGS=" /RTC1 /Z7 /errorReport:none " )

        if debugBuild:
            env.Append( LINKFLAGS=" /debug " )
            env.Append( CPPFLAGS=" /Od " )
            env.Append( CPPDEFINES=[ "_DEBUG" ] )

    if guess_arch == "ia64":
        env.Append( EXTRALIBPATH=[ winSDKHome + "/Lib/x64" ] )
        env.Append( EXTRALIBPATH=[ "C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC/lib/amd64" ] )
    else:
        env.Append( EXTRALIBPATH=[ winSDKHome + "/Lib" ] )

    winLibString = "ws2_32.lib kernel32.lib advapi32.lib Psapi.lib"
    winLibString += " user32.lib gdi32.lib winspool.lib comdlg32.lib  shell32.lib ole32.lib oleaut32.lib "
    winLibString += " odbc32.lib odbccp32.lib uuid.lib dbghelp.lib "

    env.Append( LIBS=Split(winLibString) )
elif guess_os == 'aix':
   aix = True
   nix = True

   if xlc:
      env.Replace( CC=" xlC_r -qtls " )
      env.Replace( CXX=" xlC_r -qtls " )
   # -lm
   env.Append( LIBS=['m'] )
   # -ldl
   env.Append( LIBS=['dl'] )
   # -lpthread
   env.Append( LIBS=["pthread"] )
   # GNU
   env.Append( CPPDEFINES=[ "_GNU_SOURCE" ] )
   # AIX64
   if xlc:
      env.Append( CPPFLAGS=" -q64 " )
      env.Append( LINKFLAGS=" -q64 " )
      env.Replace( SHLINKFLAGS=" -q64 -qmkshrobj " )
   else: # gcc
      env.Append( CPPFLAGS=" -maix64 -static-libgcc -static-libstdc++ " )
      env.Append( LINKFLAGS=" -maix64 -static-libgcc -static-libstdc++ " )
   env.Append( AR=" -X64 " )
   nixLibPrefix = "lib"
   # use big endian
   env.Append( CPPDEFINES=[ "SDB_BIG_ENDIAN" ] )
   env.Append( EXTRALIBPATH="/lib" )

   #env.Append( LIBS=['ssl'] )
   #env.Append( LIBS=['crypto'] )
   ssllib_file = join(ssl_lib_dir, 'libcrypto.a')
   ssllib_file1 = join(ssl_lib_dir, 'libssl.a')

   # spider monkey
   if usesm:
      smlib_file = join(sm_lib_dir, 'libmozjs185.so')
      env.Append( CPPDEFINES=[ "XP_UNIX" ] )
      env.Append( LIBS=['js_static'] )

   # lz4, zlib and snappy
   zlib_lib = join(zlib_lib_dir, 'libzlib.a')
   lz4_lib = join(lz4_lib_dir, 'liblz4.a')
   snappy_lib = join(snappy_lib_dir, 'libsnappy.a')
   zstd_lib = join(zstd_lib_dir, 'libzstd.a')
else:
    platform_valid = False
    print( "No special config for [" + os.sys.platform + "] which probably means it won't work" )



env['STATIC_AND_SHARED_OBJECTS_ARE_THE_SAME'] = 1
if nix:
    if xlc:
        env.Append( CPPFLAGS=" -qpic=large -qalias=noansi -g " )
    else:
        env.Append( CPPFLAGS="-fPIC -fno-strict-aliasing -ggdb -pthread -Wno-write-strings -Wall -Wsign-compare -Wno-unknown-pragmas -Winvalid-pch -Wno-address" )
        env.Append( CXXFLAGS=" -Wnon-virtual-dtor -fcheck-new" )
        if aix:
            env.Append( LINKFLAGS=" -fPIC -pthread " )
        else:
            env.Append( LINKFLAGS=" -fPIC -pthread -rdynamic" )
    if linux:
        env.Append( CPPFLAGS=" -pipe " )
        env.Append( CPPFLAGS=" -fno-builtin-memcmp " )
    env.Append( CPPDEFINES="_FILE_OFFSET_BITS=64" )
    env.Append( LIBS=[] )
    env['ENV']['HOME'] = os.environ['HOME']
    env['ENV']['TERM'] = os.environ['TERM']

    if debugBuild:
        env.Append( CPPFLAGS=" -O0 " )
        if not aix:
            env.Append( CPPFLAGS=" -fstack-protector " )
        env['ENV']['GLIBCXX_FORCE_NEW'] = 1
        env.Append( CPPFLAGS=" -D_DEBUG" )
    else:
        env.Append( CPPFLAGS=" -O3 " )
        env.Append( CPPFLAGS=" -DNDEBUG" )

try:
    umask = os.umask(022)
except OSError:
    pass

env.Append( CPPPATH=env["EXTRACPPPATH"], LIBPATH=env["EXTRALIBPATH"])

# --- check system ---
def getSysInfo():
    if windows:
        return "windows " + str( sys.getwindowsversion() )
    else:
        return " ".join( os.uname() )

clientCppEnv = env.Clone()
clientCppEnv.Append( CPPDEFINES=[ "SDB_DLL_BUILD" ] )
clientCEnv = clientCppEnv.Clone()
fapEnv = clientCppEnv.Clone()
fapEnv["BUILD_DIR"] = fapVariantDir
clientCppEnv["BUILD_DIR"] = clientCppVariantDir
clientCEnv["BUILD_DIR"] = clientCVariantDir

# To ensure compatibility, use the C++98 standard to compile the C++ driver
if linux:
    clientCppEnv[ 'CXXFLAGS' ].remove( '-std=c++11' )
    clientCppEnv.Append( CXXFLAGS=" -std=c++98 " )

# we do not want c/cpp client to have those "CPPFLAGS",
# so just append them here
# those flags "LINKFLAGS" will cause LNK2001 errors
# when we build dlls of cpp, so just append them here.
if windows:
    if release:
        env.Append( CPPFLAGS= " /MT " )
        env.Append( LINKFLAGS=" /NODEFAULTLIB:MSVCPRT  " )
    else:
        env.Append( CPPFLAGS= " /MDd " )
        env.Append( LINKFLAGS=" /NODEFAULTLIB:MSVCPRT  /NODEFAULTLIB:MSVCRT  " )

# --- append boost library to env ---
if nix:
   for b in boostLibs:
      env.Append ( _LIBFLAGS='${SLIBS}',
                   SLIBS=" " + join(boost_lib_dir,"libboost_" + b + ".a") )
if linux:
   # add -lrt for boost_thread.a, need clock_gettime reference
   env.Append ( _LIBFLAGS=' -lrt ' )

wtEnv = env.Clone()
wtEnv["BUILD_DIR"] = wtVariantDir

thirdpartyEnv = env.Clone()
testEnv = env.Clone()
testEnv.Append( CPPPATH=["../"] )

shellEnv = None
shellEnv = env.Clone();

toolEnv = None
toolEnv = env.Clone() ;

fmpEnv = None
fmpEnv = env.Clone() ;

env.Append( CPPPATH = wtVariantDir )
env.Append( LIBS = "wiredtiger" )
env.Append( LIBPATH = wtVariantDir )

if windows:
    env.Append( LIBS=['liblz4', 'libzlib', 'libsnappy', 'libzstd'] )
    wtEnv.Append( LIBS=['liblz4', 'libzlib', 'libsnappy', 'libzstd'] )
    shellEnv.Append( LIBS=['liblz4', 'libzlib', 'libsnappy', 'libzstd'] )
    toolEnv.Append( LIBS=['liblz4', 'libzlib', 'libsnappy', 'libzstd'] )
    fmpEnv.Append( LIBS=['liblz4', 'libzlib', 'libsnappy', 'libzstd'] )
    testEnv.Append( LIBS=['liblz4', 'libzlib', 'libsnappy', 'libzstd'] )
else:
    env.Append( LIBS=['lz4', 'zlib', 'snappy', 'zstd'] )
    wtEnv.Append( LIBS=['lz4', 'zlib', 'snappy', 'zstd'] )
    shellEnv.Append( LIBS=['lz4', 'zlib', 'snappy', 'zstd'] )
    toolEnv.Append( LIBS=['lz4', 'zlib', 'snappy', 'zstd'] )
    fmpEnv.Append( LIBS=['lz4', 'zlib', 'snappy', 'zstd'] )
    testEnv.Append( LIBS=['lz4', 'zlib', 'snappy', 'zstd'] )

if windows:
    shellEnv.Append( LIBS=["winmm.lib"] )
    #env.Append( CPPFLAGS=" /TP " )

if usemdocml:
    if windows:
        shellEnv.Append( LIBS=['libmdocml'] )
    else:
        shellEnv.Append( LIBS=['mdocml'] )
    shellEnv.Append(CPPPATH=[join(mdocml_dir, 'include')])
    shellEnv.Append(EXTRALIBPATH=[mdocml_lib_dir])
    shellEnv.Append( LIBPATH=['$EXTRALIBPATH'] )

# add engine and client variable
env.Append( CPPDEFINES=[ "SDB_ENGINE" ] )
clientCppEnv.Append( CPPDEFINES=[ "SDB_CLIENT" ] )
clientCEnv.Append( CPPDEFINES=[ "SDB_CLIENT" ] )
# should we use engine or client for test env? not sure, let's put client for now
testEnv.Append( CPPDEFINES=[ "SDB_CLIENT" ] )
shellEnv.Append( CPPDEFINES=[ "SDB_CLIENT" ] )
shellEnv.Append( CPPDEFINES=[ "SDB_SHELL" ] )
toolEnv.Append( CPPDEFINES=[ "SDB_CLIENT" ] )
toolEnv.Append( CPPDEFINES=[ "SDB_TOOL" ] )
toolEnv.Append( CPPPATH=[ncursesinclude_dir] )
fmpEnv.Append( CPPDEFINES=[ "SDB_FMP" ] )
fmpEnv.Append( CPPDEFINES=[ "SDB_CLIENT" ] )
fapEnv.Append( CPPDEFINES=["SDB_ENGINE", "SDB_DLL_BUILD"])
#fapEnv.Append( CPPPATH=[join(engine_dir, "bson")])

# drivers always set SSL definition
toolEnv.Append( CPPDEFINES=[ "SDB_SSL" ] )
clientCppEnv.Append( CPPDEFINES=[ "SDB_SSL" ] )
clientCEnv.Append( CPPDEFINES=[ "SDB_SSL" ] )
shellEnv.Append( CPPDEFINES=[ "SDB_SSL" ] )

if hasSSL:
    env.Append( CPPDEFINES=[ "SDB_SSL" ] )
    fapEnv.Append( CPPDEFINES=[ "SDB_SSL" ] )

if linux:
    if hasGProf:
        env.Append( CPPFLAGS=" -pg " )
        env.Append( LINKFLAGS=" -pg " )

env['INSTALL_DIR'] = installDir
if testEnv is not None:
    testEnv['INSTALL_DIR'] = installDir
if shellEnv is not None:
    shellEnv['INSTALL_DIR'] = installDir
if clientCppEnv is not None:
    clientCppEnv['INSTALL_DIR'] = installDir
if clientCEnv is not None:
    clientCEnv['INSTALL_DIR'] = installDir
if fmpEnv is not None:
    fmpEnv['INSTALL_DIR'] = installDir
if fapEnv is not None:
    fapEnv['INSTALL_DIR'] = installDir

if cov:
   env.Append( CPPFLAGS=" -fprofile-arcs -ftest-coverage " )
   env.Append( LINKFLAGS=" -fprofile-arcs " )
   shellEnv.Append( CPPFLAGS=" -fprofile-arcs -ftest-coverage " )
   shellEnv.Append( LINKFLAGS=" -fprofile-arcs " )
   #toolEnv.Append( CPPFLAGS=" -fprofile-arcs -ftest-coverage " )
   #toolEnv.Append( LINKFLAGS=" -fprofile-arcs " )
   fmpEnv.Append( CPPFLAGS=" -fprofile-arcs -ftest-coverage " )
   fmpEnv.Append( LINKFLAGS=" -fprofile-arcs " )
   fapEnv.Append( CPPFLAGS=" -fprofile-arcs -ftest-coverage " )
   fapEnv.Append( LINKFLAGS=" -fprofile-arcs " )


if linux64:
    toolEnv.Append( CPPDEFINES="_FILE_OFFSET_BITS=64" )
if usefuse:
    toolEnv.Append( LIBS=['fuse'] )
    toolEnv.Append( CPPPATH = join(fuse_dir, "include") )
    toolEnv.Append( EXTRALIBPATH=[fuse_lib_dir] )
toolEnv.Append( LIBPATH=['$EXTRALIBPATH'] )

# The following symbols are exported for use in subordinate SConscript files.
# Ideally, the SConscript files would be purely declarative.  They would only
# import build environment objects, and would contain few or no conditional
# statements or branches.
#
# Currently, however, the SConscript files do need some predicates for
# conditional decision making that hasn't been moved up to this SConstruct file,
# and they are exported here, as well.
Export("env")
Export("thirdpartyEnv")
Export("shellEnv")
Export("toolEnv")
Export("testEnv")
Export("fmpEnv")
Export("fapEnv")
Export("clientCppEnv")
Export("clientCEnv")
Export("wtEnv")
Export("installSetup getSysInfo")
Export("usesm")
Export("usefuse")
Export("usemdocml")
Export("windows linux nix aix linux64")
if usesm:
   Export("smlib_file")
Export("ssllib_file")
Export("ssllib_file1")
Export("zlib_lib")
Export("lz4_lib")
Export("snappy_lib")
Export("zstd_lib")
if usemdocml:
   Export("mdocml_lib")
if usefuse:
   Export("fuse_lib")
Export("hasEngine")
Export("hasTestcase")
Export("hasTool")
Export("driverDir")
Export("guess_os")
Export("guess_arch")
Export("mergeStaticLibrary")
Export("hasSSL")
Export("release")
Export("debugBuild")
Export("cov")
Export("boost_lib_dir")
Export("intel_decimal_lib_dir")
Export("wiredtiger_dir")
Export("wtVariantDir")

print("Begin to build thirdparty...")
thirdpartyEnv.SConscript('thirdparty/SConscript', exports=["boost_lib_dir",
                         "ssl_lib_dir", "zlib_dir", "zlib_lib_dir",
                         "lz4_dir", "lz4_lib_dir", "snappy_dir", "snappy_lib_dir",
                         "zstd_dir", "zstd_lib_dir",
                         "sm_lib_dir", "mdocml_lib_dir", "fuse_lib_dir", "intel_decimal_lib_dir",
                         "wiredtiger_dir"], duplicate=False)

if not has_option("noautogen"):
   language = get_option ( "language" )
   autogen_result = 0
   silent_opt = ''
   if has_option("silent"):
      silent_opt = "--silent"

   if language is None:
      autogen_result = os.system ( "scons -C misc/autogen" + " " + silent_opt )
   else:
      autogen_result = os.system ( "scons -C misc/autogen --language=" +
                                   language + " " + silent_opt)
   if autogen_result != 0:
      os._exit( 1 )
   version_file = 'misc/autogen/version.py'
   shutil.copyfile(version_file, 'tools/sdbaudit/version.py')
   shutil.copyfile(version_file, 'driver/python/version.py')

if hasDoc:
   errno = os.system ( 'python doc/build.py --doc' )
   os._exit( errno )

if hasWebSite:
   errno = os.system ( 'python doc/build.py --website' )
   os._exit( errno )

if hasChm:
   errno = os.system ( 'python doc/build.py --chm' )
   os._exit( errno )

if hasOffline:
   errno = os.system ( 'python doc/build.py --offline' )
   os._exit( errno )

if hasDoxygen:
   errno = os.system ( 'python doc/build.py --doxygen' )
   os._exit( errno )

if hasEngine:
   wtEnv.SConscript('thirdparty/wiredtiger/SConscript',
                    variant_dir=wtVariantDir,
                    duplicate=False)
   env.SConscript( 'SequoiaDB/SConscript', variant_dir=variantDir, duplicate=False )

if hasClient:
   if not xlc: # xlc doesn't support #pragma once, so there are compiling errors
       clientCppEnv.SConscript( 'SequoiaDB/SConscriptClientCpp', variant_dir=clientCppVariantDir, duplicate=False )
   clientCEnv.SConscript ( 'SequoiaDB/SConscriptClientC', variant_dir=clientCVariantDir, duplicate=False )

if hasShell:
   shellEnv.SConscript ( 'SequoiaDB/SConscriptShell', variant_dir=shellVariantDir, duplicate=False )

if hasTool:
   toolEnv.SConscript ( 'SequoiaDB/SConscriptTool', variant_dir=toolVariantDir, duplicate=False )
if hasFmp:
   fmpEnv.SConscript ( 'SequoiaDB/SConscriptFmp', variant_dir=fmpVariantDir, duplicate=False )
#if hasTestcase:
#   env.SConscript( 'SequoiaDB/SConscript', variant_dir=variantDir, duplicate=False )
if hasFap:
   fapEnv.SConscript ( 'SequoiaDB/SConscriptFap', variant_dir=fapVariantDir, duplicate=False )

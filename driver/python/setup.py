#   Copyright (C) 2012-2014 SequoiaDB Ltd.
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

import glob
import os
import shutil
import sys
from distutils.core import setup

from version import get_version

if 'win32' == sys.platform:
    dlls = './pysequoiadb/*.dll'
    bson = './bson/*.dll'
    for file in glob.glob(dlls):
        if file.startswith('lib'):
            newname = file[3:]
        newname = file[:-3] + 'pyd'
        shutil.copy(file, newname)

    for file in glob.glob(bson):
        if file.startswith('lib'):
            newname = file[3:]
        newname = file[:-3] + 'pyd'
        shutil.copy(file, newname)

    libsdb = 'sdb.pyd'
    libdecimal = 'bsondecimal.pyd'
else:
    libsdb = 'sdb.so'
    libdecimal = 'bsondecimal.so'

extra_opts = {}
extra_opts['packages'] = ['bson', 'pysequoiadb']
extra_opts['package_dir'] = {'pysequoiadb': 'pysequoiadb', 'bson': 'bson'}
extra_opts['package_data'] = {'pysequoiadb': [libsdb],
                              'bson': ['buffer.h',
                                       'buffer.c',
                                       '_cbsonmodule.h',
                                       '_cbsonmodule.c',
                                       'encoding_helpers.h',
                                       'encoding_helpers.c',
                                       'time64.h',
                                       'time64.c',
                                       'time64_config.h',
                                       'time64_limits.h',
                                       libdecimal],}
# extra_opts['ext_modules'] = ext_modules
setup(name='pysequoiadb',
      version=get_version(),
      author='SequoiaDB Inc.',
      license='Apache License 2',
      description='This is a sequoiadb python driver use adapter package',
      url='http://www.sequoiadb.com',
      **extra_opts)

if 'win32' == sys.platform:
    pyds = './pysequoiadb/*.pyd'
    for file in glob.glob(pyds):
        os.remove(file)

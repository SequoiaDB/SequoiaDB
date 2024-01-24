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

"""Python Driver for SequoiaDB

   All operation need deal with the error code returned first, if it has. 
   Every error code is not SDB_OK(or 0), it means something error has appeared,
   and user should deal with it according the meaning of error code printed.

   @version: execute to get version
             >>> import pysequoiadb
             >>> print pysequoiadb.get_version()

   @notice : The dict of built-in Python is hashed and non-ordered. so the
             element in dict may not the order we make it. we make a dict and
             print it like this:
             ...
             >>> a = {"avg_age":24, "major":"computer science"}
             >>> a
             >>> {'major': 'computer science', 'avg_age': 24}
             ...
             the elements order it is not we make it!!
             therefore, we use bson.SON to make the order-sensitive dict if the
             order is important such as operations in "$sort", "$group",
             "split_by_condition", "aggregate","create_collection"...
             In every scene which the order is important, please make it using
             bson.SON and list. It is a subclass of built-in dict
             and order-sensitive
"""

import sys

from pysequoiadb.client import client
from pysequoiadb.error import (SDBBaseError,
                               SDBTypeError,
                               SDBEndOfCursor,
                               SDBIOError,
                               SDBNetworkError,
                               SDBInvalidArgument,
                               SDBSystemError,
                               SDBUnknownError,
                               SDBError)

try:
    from . import sdb
except:
    raise Exception("Cannot find extension: sdb")


def get_version():
    version = sdb.sdb_get_version()
    if 5 == len(version):
        ver, sub_version, fixed, release, build = sdb.sdb_get_version()
        return ("( Version: %s, subVersion: %s, fixed: %s, Release: %s , build: %s )"
                % (ver, sub_version, fixed, release, build))
    else:
        ver, sub_version, fixed, release, build, git_ver = sdb.sdb_get_version()
        return ("( Version: %s, subVersion: %s, fixed: %s, Release: %s , build: %s, gitVersion: %s )"
                % (ver, sub_version, fixed, release, build, git_ver))


PY3 = sys.version_info[0] == 3

driver_version = get_version()
"""Current version of python driver for SequoiaDB."""


def init_client(on, cache_time_interval=0, max_cache_slot=0):
    """enable cache strategy to improve performance

    Parameters:
          Name                Type      Info:
          on                  boolean   The flag to OPEN the cache strategy
          cache_time_interval int       The life cycle of cached object
          max_cache_slot      int       The count of slot to cache objects,
                                              one slot holds an object
    """
    enable = on and 1 or 0
    sdb.sdb_init_client(enable, cache_time_interval, max_cache_slot)

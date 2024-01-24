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

"""Module of sequence for python driver of SequoiaDB
"""

try:
    from . import sdb
except:
    raise Exception("Cannot find extension: sdb")

import bson
from bson.py3compat import (PY3, str_type, long_type)
from pysequoiadb.error import (SDBTypeError,
                               SDBSystemError,
                               raise_if_error)
from pysequoiadb.errcode import (SDB_OOM)

class sequence(object):
    """Sequence for SequoiaDB
    """

    def __init__(self):
        """create a new sequence.
        """
        try:
            self._seq = sdb.create_seq()
        except SystemError:
            raise SDBSystemError(SDB_OOM, "Failed to alloc sequence")

    def __del__(self):
        """delete a sequence.
        """
        if self._seq is not None:
            rc = sdb.release_seq(self._seq)
            raise_if_error(rc, "Failed to release sequence")
            self._seq = None

    def fetch(self, fetch_num):
        """Fetch a bulk of continuous values.

        Parameters:
           Name          Type    Info
           fetch_num     int     The number of values to be fetched.
        Return values:
           A dict object that contains the following fields:
           - next_value  int     The next value and also the first returned value.
           - return_num  int     The number of values returned.
           - increment   int     Increment of values.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(fetch_num, int):
            raise SDBTypeError("fetch_num must be an instance of int")
        rc, next_value, return_num, increment  = sdb.seq_fetch(self._seq, fetch_num)
        raise_if_error(rc, "Failed to fetch continuous values for the sequence, fetch_num: %s" % fetch_num)
        result = {}
        result["next_value"] = next_value
        result["return_num"] = return_num
        result["increment"] = increment
        return result

    def get_current_value(self):
        """Get the current value of sequence.

        Return values:
           The current value
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        rc, value = sdb.seq_get_current_value(self._seq)
        raise_if_error(rc, "Failed to get the current value of sequence")
        return value

    def get_next_value(self):
        """Get the next value of sequence.

        Return values:
           The next value
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        rc, value = sdb.seq_get_next_value(self._seq)
        raise_if_error(rc, "Failed to get the next value of sequence")
        return value

    def restart(self, start_value):
        """Restart sequence from the given value.

        Parameters:
           Name          Type    Info
           start_value   int     The start value
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(start_value, int):
            raise SDBTypeError("start value must be an instance of int")
        rc = sdb.seq_restart(self._seq, start_value)
        raise_if_error(rc, "Failed to restart sequence, start value: %s" % start_value)

    def set_attributes(self, options):
        """Alter sequence.

        Parameters:
           Name              Type    Info
           options           dict    The options of sequence to be changed:
           - CurrentValue    int     The current value of sequence
           - StartValue      int     The start value of sequence
           - MinValue        int     The minimum value of sequence
           - MaxValue        int     The maxmun value of sequence
           - Increment       int     The increment value of sequence
           - CacheSize       int     The cache size of sequence
           - AcquireSize     int     The acquire size of sequence
           - Cycled          bool    The cycled flag of sequence
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(options, dict):
            raise SDBTypeError("options must be an instance of dict")
        bson_options = bson.BSON.encode(options)

        rc = sdb.seq_set_attributes(self._seq, bson_options)
        raise_if_error(rc, "Failed to alter sequence")


    def set_current_value(self, value):
        """Set the current value to sequence.

        Parameters:
           Name    Type    Info
           value   int     The expected current value
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(value, int):
            raise SDBTypeError("The value param must be an instance of int")
        rc = sdb.seq_set_current_value(self._seq, value)
        raise_if_error(rc, "Failed to set the current value to sequence, value: %s" % value)

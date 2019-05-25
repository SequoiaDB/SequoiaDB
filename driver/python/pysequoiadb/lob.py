#   Copyright (C) 2012-2017 SequoiaDB Ltd.
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

"""Module of lob for python driver of SequoiaDB
"""
try:
    from . import sdb
except:
    raise Exception("Cannot find extension: sdb")

import bson
from bson.py3compat import (str_type, long_type)
from pysequoiadb.errcode import (SDB_OOM, SDB_INVALIDARG)
from pysequoiadb.error import (SDBTypeError, SDBSystemError,
                               SDBInvalidArgument, raise_if_error)

LOB_READ = int(0x00000004)
LOB_WRITE = int(0x00000008)


class lob(object):
    def __init__(self):
        try:
            self._handle = sdb.create_lob()
        except SystemError:
            raise SDBSystemError(SDB_OOM, "Failed to create lob")

    def __del__(self):
        if self._handle is not None:
            rc = sdb.release_lob(self._handle)
            raise_if_error(rc, "Failed to release lob")
            self._handle = None

    def close(self):
        """close lob

        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        rc = sdb.lob_close(self._handle)
        raise_if_error(rc, "Failed to close lob")

    def get_size(self):
        """get the size of lob.

        Return Values:
           the size of current lob
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        rc, size = sdb.lob_get_size(self._handle)
        raise_if_error(rc, "Failed to get size of lob")
        return size

    def get_oid(self):
        """get the oid of lob.

        Return Values:
           the oid of current lob
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        rc, id_str = sdb.lob_get_oid(self._handle)
        raise_if_error(rc, "Failed to get oid of lob")
        oid = bson.ObjectId(id_str)
        return oid

    def get_create_time(self):
        """get create time of lob

        Return Values:
           a long int of time
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        rc, mms = sdb.lob_get_create_time(self._handle)
        raise_if_error(rc, "Failed to get create time of lob")
        return mms

    def get_modification_time(self):
        """get the last modification time of lob

        Return Values:
           a long int of time
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        rc, mms = sdb.lob_get_modification_time(self._handle)
        raise_if_error(rc, "Failed to get modification time of lob")
        return mms

    def lock(self, offset, length):
        """lock lob data section.

        Parameters:
            Name        Type                Info:
           offset    long(int in python3)   The lock start position
           length    long(int in python3)   The lock length, -1 means lock from offset to the end of lob
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(offset, (int, long_type)):
            raise SDBTypeError("seek_pos must be an instance of long/int")
        if not isinstance(length, (int, long_type)):
            raise SDBTypeError("seek_pos must be an instance of long/int")

        rc = sdb.lob_lock(self._handle, offset, length)
        raise_if_error(rc, "Failed to lock lob")

    def lock_and_seek(self, offset, length):
        """lock lob data section and seek to the offset position.

        Parameters:
            Name        Type                Info:
           offset    long(int in python3)   The lock start position
           length    long(int in python3)   The lock length, -1 means lock from offset to the end of lob
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(offset, (int, long_type)):
            raise SDBTypeError("seek_pos must be an instance of long/int")
        if not isinstance(length, (int, long_type)):
            raise SDBTypeError("seek_pos must be an instance of long/int")

        rc = sdb.lob_lock_and_seek(self._handle, offset, length)
        raise_if_error(rc, "Failed to lock lob")

    def seek(self, seek_pos, whence=0):
        """seek in lob.

        Parameters:
           Name        Type           Info:
           seek_pos    int            The length to seek
           whence      int            whence of seek, it must be 0/1/2
                                            0 means seek from begin to end of lob
                                            1 means seek from currend position to end of lob
                                            2 means seek from end to begin of lob
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(seek_pos, int):
            raise SDBTypeError("seek_pos must be an instance of int")
        if not isinstance(whence, int):
            raise SDBTypeError("seek_pos must be an instance of int")
        if whence not in (0, 1, 2):
            raise SDBInvalidArgument(SDB_INVALIDARG, "value of whence is in valid")

        rc = sdb.lob_seek(self._handle, seek_pos, whence)
        raise_if_error(rc, "Failed to seek lob")

    def read(self, length):
        """ream data from lob.

        Parameters:
           Name     Type                 Info:
           length   int                  The length of data to be read
        Return Values:
           binary data of read
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(length, int):
            raise SDBTypeError("len must be an instance of int")

        rc, data = sdb.lob_read(self._handle, length)
        raise_if_error(rc, "Failed to read data from lob")
        return data

    def write(self, data, length):
        """write data into lob.

        Parameters:
           Name     Type                 Info:
           data     str                  The data to be written
           length   int                  The length of data to be written
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(data, str_type):
            raise SDBTypeError("data should be byte or string")

        rc = sdb.lob_write(self._handle, data, length)
        raise_if_error(rc, "Failed to write data to lob")

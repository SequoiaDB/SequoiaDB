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

try:
    from . import sdb
except:
    raise Exception("Cannot find extension: sdb")

from collections import OrderedDict

import bson
from pysequoiadb.errcode import SDB_OOM
from pysequoiadb.error import (SDBSystemError, SDBTypeError, raise_if_error)


class cursor(object):
    """Cursor of SequoiaDB

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

    def __init__(self):
        """constructor of cursor

        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        self._cursor = None
        try:
            self._cursor = sdb.create_cursor()
        except SystemError:
            raise SDBSystemError(SDB_OOM, "Failed to alloc cursor")

    def __del__(self):
        """release cursor

        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if self._cursor is not None:
            rc = sdb.release_cursor(self._cursor)
            raise_if_error(rc, "Failed to release cursor")
            self._cursor = None

    def next(self, ordered=False):
        """Return the next document of current cursor, and move forward.

        Parameters:
           Name      Type  Info:
           ordered   bool  Set true if need field-ordered records, default false.

        Return values:
           a dict object of record
        Exceptions:
           pysequoiadb.error.SDBEndOfCursor
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(ordered, bool):
            raise SDBTypeError("ordered must be an instance of bool")

        as_class = dict
        if ordered:
            as_class = OrderedDict

        rc, bson_string = sdb.cr_next(self._cursor)
        raise_if_error(rc, "Failed to get next record")
        record, size = bson._bson_to_dict(bson_string, as_class, False,
                                          bson.OLD_UUID_SUBTYPE, True)
        return record

    def current(self, ordered=False):
        """Return the current document of cursor, and don't move.

        Parameters:
           Name      Type  Info:
           ordered   bool  Set true if need field-ordered records, default false.

        Return values:
           a dict object of record
        Exceptions:
           pysequoiadb.error.SDBEndOfCursor
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(ordered, bool):
            raise SDBTypeError("ordered must be an instance of bool")

        as_class = dict
        if ordered:
            as_class = OrderedDict

        rc, bson_string = sdb.cr_current(self._cursor)
        raise_if_error(rc, "Failed to get current record")
        record, size = bson._bson_to_dict(bson_string, as_class, False,
                                          bson.OLD_UUID_SUBTYPE, True)
        return record

    def close(self):
        """Close the cursor's connection to database, we can't use this handle to
           get data again.

        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        rc = sdb.cr_close(self._cursor)
        raise_if_error(rc, "Failed to close cursor")

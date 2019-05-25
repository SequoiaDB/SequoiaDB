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

"""Module of collectionspace for python driver of SequoiaDB
"""

try:
    from . import sdb
except:
    raise Exception("Cannot find extension: sdb")

import bson
import pysequoiadb
from bson.py3compat import (PY3, str_type)
from pysequoiadb.collection import collection
from pysequoiadb.cursor import cursor
from pysequoiadb import error
from pysequoiadb.error import (SDBBaseError, SDBSystemError, SDBTypeError, raise_if_error)
from pysequoiadb.errcode import SDB_OOM


class collectionspace(object):
    """CollectionSpace for SequoiaDB

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
        """invoked when a new object is producted.

        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        # 'cs' is short for collection space
        try:
            self._cs = sdb.create_cs()
        except SystemError:
            raise SDBSystemError(SDB_OOM, "Failed to alloc collection space")

    def __del__(self):
        """delete a object existed.

        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if self._cs is not None:
            rc = sdb.release_cs(self._cs)
            raise_if_error(rc, "Failed to release collection space")
            self._cs = None

    def __repr__(self):
        return "Collection Space: %s" % (self.get_collection_space_name())

    def __getattr__(self, name):
        """support client.cs to access to collection.

           eg.
           cc = client()
           cs = cc.test
           cl = cs.test_cl  # access to collection named 'test_cl'

           and we should pass '__members__' and '__methods__',
           becasue dir(cc) will invoke __getattr__("__members__") and
           __getattr__("__methods__").

           if success, a collection object will be returned.

        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if '__members__' == name or '__methods__' == name:
            pass
        else:
            cl = collection()
            try:
                rc = sdb.cs_get_collection(self._cs, name, cl._cl)
                raise_if_error(rc, "Failed to get collection: %s" % name)
            except SDBBaseError:
                del cl
                raise

            return cl

    def __getitem__(self, name):
        """support [] to access to collection.

           eg.
           cc = client()
           cs = cc['test']
           cl = cs['test_cl']   # access to collection named 'test_cl'.

        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        return self.__getattr__(name)

    def get_collection(self, cl_name):
        """Get the named collection.

        Parameters:
           Name         Type     Info:
           cl_name      str      The short name of the collection.
        Return values:
           a collection object of query
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(cl_name, str_type):
            raise SDBTypeError("collection must be an instance of str_type")

        cl = collection()
        try:
            rc = sdb.cs_get_collection(self._cs, cl_name, cl._cl)
            raise_if_error(rc, "Failed to get collection: %s" % cl_name)
        except SDBBaseError:
            del cl
            raise

        return cl

    def create_collection(self, cl_name, options=None):
        """create a collection using name and options.

        Parameters:
           Name      Type     Info:
           cl_name   str      The collection name.
           options   dict     The options for creating collection, including
                                    "ShardingKey", "ReplSize", "IsMainCL" and
                                    "Compressed" informations, no options, if None.
        Return values:
           a collection object created
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(cl_name, str_type):
            raise SDBTypeError("collection must be an instance of str_type")

        bson_options = None
        if options is not None:
            if not isinstance(options, dict):
                raise SDBTypeError("options must be an instance of dict")
            bson_options = bson.BSON.encode(options)

        cl = collection()
        try:
            if bson_options is None:
                rc = sdb.cs_create_collection(self._cs, cl_name, cl._cl)
            else:
                rc = sdb.cs_create_collection_use_opt(self._cs, cl_name,
                                                      bson_options, cl._cl)
            raise_if_error(rc, "Failed to create collection")
        except SDBBaseError:
            del cl
            raise

        return cl

    def drop_collection(self, cl_name):
        """Drop the specified collection in current collection space.

        Parameters:
           Name      Type     Info:
           cl_name   str      The collection name.
        Exceptions:
           pysequoiadb.error.SDBTypeError
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(cl_name, str_type):
            raise SDBTypeError("collection must be an instance of str_type")

        rc = sdb.cs_drop_collection(self._cs, cl_name)
        raise_if_error(rc, "Failed to drop collection")

    def get_collection_space_name(self):
        """Get the current collection space name.

        Return values:
           The name of current collection space.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        rc, cs_name = sdb.cs_get_collection_space_name(self._cs)
        raise_if_error(rc, "Failed to get collection space name")

        return cs_name

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

"""Module of collection for python driver of SequoiaDB
"""

try:
    from . import sdb
except:
    raise Exception("Cannot find extension: sdb")

import bson
from bson.objectid import ObjectId
import pysequoiadb
from bson.py3compat import (PY3, str_type, long_type)
from pysequoiadb.cursor import cursor
from pysequoiadb.lob import (lob, LOB_READ, LOB_WRITE)
from pysequoiadb.error import (SDBBaseError,
                               SDBTypeError,
                               SDBSystemError,
                               SDBEndOfCursor,
                               SDBInvalidArgument,
                               raise_if_error)
from pysequoiadb.errcode import (SDB_OOM, SDB_INVALIDARG)

QUERY_FLG_WITH_RETURNDATA = 0x00000080
QUERY_FLG_PARALLED = 0x00000100
QUERY_FLG_FORCE_HINT = 0x00000200
QUERY_PREPARE_MORE = 0x00004000
QUERY_FLG_KEEP_SHARDINGKEY_IN_UPDATE = 0x00008000

UPDATE_FLG_KEEP_SHARDINGKEY = QUERY_FLG_KEEP_SHARDINGKEY_IN_UPDATE


class collection(object):
    """Collection for SequoiaDB

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
        """create a new collection.

        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        try:
            self._cl = sdb.create_cl()
        except SystemError:
            raise SDBSystemError(SDB_OOM, "Failed to alloc collection")

    def __del__(self):
        """delete a object existed.

        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if self._cl is not None:
            rc = sdb.release_cl(self._cl)
            raise_if_error(rc, "Failed to release collection")
            self._cl = None

    def __repr__(self):
        return "Collection: %s" % (self.get_full_name())

    def get_count(self, condition=None):
        """Get the count of matching documents in current collection.

        Parameters:
           Name         Type     Info:
           condition    dict     The matching rule, return the count of all
                                       documents if None.
        Return values:
           count of result
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        bson_condition = None
        if condition is not None:
            if not isinstance(condition, dict):
                raise SDBTypeError("condition must be an instance of dict")
            bson_condition = bson.BSON.encode(condition)

        rc, count = sdb.cl_get_count(self._cl, bson_condition)
        raise_if_error(rc, "Failed to get count of record")

        return count

    def split_by_condition(self, source_group_name, target_group_name,
                           split_condition,
                           split_end_condition=None):
        """Split the specified collection from source replica group to target
           replica group by range.

        Parameters:
           Name                  Type     Info:
           source_group_name     str      The source replica group name.
           target_group_name     str      The target replica group name.
           split_condition       dict     The matching rule, return the count
                                                of all documents if None.
           split_end_condition   dict     The split end condition or None.
                                                eg:
                                                If we create a collection with the
                                                option { ShardingKey:{"age":1},
                                                ShardingType:"Hash",Partition:2^10 },
                                                we can fill {age:30} as the
                                                splitCondition, and fill {age:60}
                                                as the splitEndCondition. when
                                                split, the target replica group
                                                will get the records whose age's
                                                hash value are in [30,60).
                                                If splitEndCondition is null, they
                                                are in [30,max).
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(source_group_name, str_type):
            raise SDBTypeError("source group name must be an instance of str_type")
        if not isinstance(target_group_name, str_type):
            raise SDBTypeError("target group name must be an instance of str_type")

        bson_split_condition = None
        if split_condition is not None:
            if not isinstance(split_condition, dict):
                raise SDBTypeError("split condition must be an instance of dict")
            bson_split_condition = bson.BSON.encode(split_condition)

        bson_end_condition = None
        if split_end_condition is not None:
            if not isinstance(split_end_condition, dict):
                raise SDBTypeError("split end condition must be an instance of dict")
            bson_end_condition = bson.BSON.encode(split_end_condition)

        rc = sdb.cl_split_by_condition(self._cl, source_group_name,
                                       target_group_name,
                                       bson_split_condition,
                                       bson_end_condition)
        raise_if_error(rc, "Failed to split")

    def split_by_percent(self, source_group_name, target_group_name, percent):
        """Split the specified collection from source replica group to target
           replica group by percent.

        Parameters:
           Name               Type     Info:
           source_group_name  str      The source replica group name.
           target_group_name  str      The target replica group name.
           percent	          float    The split percent, Range:(0,100]
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(source_group_name, str_type):
            raise SDBTypeError("source group name must be an instance of str_type")
        if not isinstance(target_group_name, str_type):
            raise SDBTypeError("target group name must be an instance of str_type")
        if not isinstance(percent, float) and not isinstance(percent, int):
            raise SDBTypeError("percent must be an instance of float or int values in (0, 100]")

        rc = sdb.cl_split_by_percent(self._cl, source_group_name,
                                     target_group_name, percent)
        raise_if_error(rc, "Failed to split by percent")

    def split_async_by_condition(self, source_group_name, target_group_name,
                                 split_condition, split_end_condition=None):
        """Split the specified collection from source replica group to target
           replica group by range.

        Parameters:
           Name                  Type  Info:
           source_group_name     str   The source replica group name.
           target_group_name     str   The target replica group name.
           split_condition       dict  The matching rule, return the count of
                                             all documents if None.
           split_end_condition   dict  The split end condition or None.
                                             eg:
                                             If we create a collection with the
                                             option { ShardingKey:{"age":1},
                                             ShardingType:"Hash",Partition:2^10 },
                                             we can fill {age:30} as the
                                             splitCondition, and fill {age:60}
                                             as the splitEndCondition. when split,
                                             the target replica group will get the
                                             records whose age's hash value are in
                                             [30,60). If splitEndCondition is null,
                                             they are in [30,max).
        Return values:
           task id
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(source_group_name, str_type):
            raise SDBTypeError("source group name must be an instance of str_type")
        if not isinstance(target_group_name, str_type):
            raise SDBTypeError("target group name must be an instance of str_type")

        bson_split_condition = None
        if split_condition is not None:
            if not isinstance(split_condition, dict):
                raise SDBTypeError("split condition must be an instance of dict")
            bson_split_condition = bson.BSON.encode(split_condition)

        bson_end_condition = None
        if split_end_condition is not None:
            if not isinstance(split_end_condition, dict):
                raise SDBTypeError("split end condition must be an instance of dict")
            bson_end_condition = bson.BSON.encode(split_end_condition)

        rc, task_id = sdb.cl_split_async_by_condition(self._cl,
                                                      source_group_name,
                                                      target_group_name,
                                                      bson_split_condition,
                                                      bson_end_condition)
        raise_if_error(rc, "Failed to split async")

        return task_id

    def split_async_by_percent(self, source_group_name, target_group_name,
                               percent):
        """Split the specified collection from source replica group to target
           replica group by percent.

        Parameters:
           Name               Type     Info:
           source_group_name  str      The source replica group name.
           target_group_name  str      The target replica group name.
           percent	          float    The split percent, Range:(0,100]
        Return values:
           task id
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(source_group_name, str_type):
            raise SDBTypeError("source group name must be an instance of str_type")
        if not isinstance(target_group_name, str_type):
            raise SDBTypeError("target group name must be an instance of str_type")
        if not isinstance(percent, float):
            raise SDBTypeError("percent must be an instance of float")

        rc, task_id = sdb.cl_split_async_by_percent(self._cl,
                                                    source_group_name,
                                                    target_group_name,
                                                    percent)
        raise_if_error(rc, "Failed to split async")

        return task_id

    def bulk_insert(self, flags, records):
        """Insert a bulk of record into current collection.

        Parameters:
           Name        Type       Info:
           flags       int        0 or 1, see Info as below.
           records     list/tuple The list of inserted records.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        Info:
           flags : 0 or 1.
           0 : stop insertting when hit index key duplicate error
           1 : continue insertting records even though index key duplicate error hit
        """
        if not isinstance(flags, int):
            raise SDBTypeError("flags must be an instance of int")

        container = []
        for elem in records:
            if not isinstance(elem, dict):
                raise SDBTypeError("record must be an instance of dict")
            record = bson.BSON.encode(elem)
            container.append(record)

        rc = sdb.cl_bulk_insert(self._cl, flags, container)
        raise_if_error(rc, "Failed to insert records")

    def insert(self, record):
        """Insert a record into current collection.

        Parameters:
           Name      Type    Info:
           records   dict    The inserted record.
        Return values:
           ObjectId of record inserted
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(record, dict):
            raise SDBTypeError("record must be an instance of dict")

        bson_record = bson.BSON.encode(record)
        rc, id_str = sdb.cl_insert(self._cl, bson_record)
        raise_if_error(rc, "Failed to insert record")
        oid = bson.ObjectId(id_str)
        return oid

    def update(self, rule, **kwargs):
        """Update the matching documents in current collection.

        Parameters:
           Name        Type     Info:
           rule        dict     The updating rule.
           **kwargs             Useful option are below
           - condition dict     The matching rule, update all the documents
                                      if not provided.
           - hint      dict     The hint, automatically match the optimal hint
                                      if not provided
           - flags     int      The update flag
        Exceptions:
           pysequoiadb.error.SDBBaseError
        Info:
           query flags:
           UPDATE_FLG_KEEP_SHARDINGKEY : The sharding key in update rule is not filtered, when executing
                                               update or upsert.
        Note:
           When flag is set to 0, it won't work to update the "ShardingKey" field, but the
           other fields take effect.
        """
        if not isinstance(rule, dict):
            raise SDBTypeError("rule must be an instance of dict")

        bson_rule = bson.BSON.encode(rule)
        bson_condition = None
        bson_hint = None
        flags = 0

        if "condition" in kwargs:
            if not isinstance(kwargs.get("condition"), dict):
                raise SDBTypeError("condition in kwargs must be an instance of dict")
            bson_condition = bson.BSON.encode(kwargs.get("condition"))
        if "hint" in kwargs:
            if not isinstance(kwargs.get("hint"), dict):
                raise SDBTypeError("hint in kwargs must be an instance of dict")
            bson_hint = bson.BSON.encode(kwargs.get("hint"))
        if "flags" in kwargs:
            if not isinstance(kwargs.get("flags"), int):
                raise SDBTypeError("flags must be an instance of int")
            else:
                flags = kwargs.get("flags")

        rc = sdb.cl_update(self._cl, bson_rule, bson_condition, bson_hint, flags)
        raise_if_error(rc, "Failed to update")

    def upsert(self, rule, **kwargs):
        """Update the matching documents in current collection, insert if
           no matching.

        Parameters:
           Name          Type  Info:
           rule          dict  The updating rule.
           **kwargs            Useful options are below
           - condition   dict  The matching rule, update all the documents
                                     if not provided.
           - hint        dict  The hint, automatically match the optimal hint
                                     if not provided
           - setOnInsert dict  The setOnInsert assigns the specified values
                               to the fileds when insert
           - flags       int   The update flag
        Exceptions:
           pysequoiadb.error.SDBBaseError
        Info:
           query flags:
           UPDATE_FLG_KEEP_SHARDINGKEY : The sharding key in update rule is not filtered, when executing
                                               update or upsert.
        Note:
           When flag is set to 0, it won't work to update the "ShardingKey" field, but the
           other fields take effect.
        """
        if not isinstance(rule, dict):
            raise SDBTypeError("rule must be an instance of dict")
        bson_rule = bson.BSON.encode(rule)

        bson_condition = None
        bson_hint = None
        bson_setOnInsert = None
        flags = 0

        if "condition" in kwargs:
            if not isinstance(kwargs.get("condition"), dict):
                raise SDBTypeError("condition must be an instance of dict")
            bson_condition = bson.BSON.encode(kwargs.get("condition"))
        if "hint" in kwargs:
            if not isinstance(kwargs.get("hint"), dict):
                raise SDBTypeError("hint must be an instance of dict")
            bson_hint = bson.BSON.encode(kwargs.get("hint"))
        if "setOnInsert" in kwargs:
            if not isinstance(kwargs.get("setOnInsert"), dict):
                raise SDBTypeError("setOnInsert must be an instance of dict")
            bson_setOnInsert = bson.BSON.encode(kwargs.get("setOnInsert"))
        if "flags" in kwargs:
            if not isinstance(kwargs.get("flags"), int):
                raise SDBTypeError("flags must be an instance of int")
            else:
                flags = kwargs.get("flags")

        rc = sdb.cl_upsert(self._cl, bson_rule, bson_condition, bson_hint,
                           bson_setOnInsert, flags)
        raise_if_error(rc, "Failed to update")

    def save(self, doc):
        """save a documents in current collection, insert if no(matching) _id.
        Parameters:
           Name          Type  Info:
           doc           dict  The updating rule.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        Note:
           It won't work to update the "ShardingKey" field, but the other fields
                 take effect.
        """
        if not isinstance(doc, dict):
            raise SDBTypeError("rule must be an instance of dict")

        if "_id" in doc:
            oid = doc.pop("_id")
            return self.upsert({"$set": doc}, condition={"_id": oid})
        else:
            return self.insert(doc)

    def delete(self, **kwargs):
        """Delete the matching documents in current collection.

        Parameters:
           Name        Type  Info:
           **kwargs          Useful options are below
           - condition dict  The matching rule, delete all the documents
                                   if not provided.
           - hint      dict  The hint, automatically match the optimal hint
                                   if not provided
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        bson_condition = None
        bson_hint = None

        if "condition" in kwargs:
            if not isinstance(kwargs.get("condition"), dict):
                raise SDBTypeError("condition must be an instance of dict")
            bson_condition = bson.BSON.encode(kwargs.get("condition"))
        if "hint" in kwargs:
            if not isinstance(kwargs.get("hint"), dict):
                raise SDBTypeError("hint must be an instance of dict")
            bson_hint = bson.BSON.encode(kwargs.get("hint"))

        rc = sdb.cl_delete(self._cl, bson_condition, bson_hint)
        raise_if_error(rc, "Failed to delete")

    def query(self, **kwargs):
        """Get the matching documents in current collection.

        Parameters:
           Name              Type     Info:
           **kwargs                   Useful options are below
           - condition       dict     The matching rule, update all the
                                            documents if not provided.
           - selector        dict     The selective rule, return the whole
                                            document if not provided.
           - order_by        dict     The ordered rule, result set is unordered
                                            if not provided.
           - hint            dict     The hint, automatically match the optimal
                                            hint if not provided.
           - num_to_skip     long     Skip the first numToSkip documents,
                                            default is 0L.
           - num_to_return   long     Only return numToReturn documents,
                                            default is -1L for returning
                                            all results.
           - flags           int      The query flags, default to be 0. Please see
                                            the definition of follow flags for
                                            more detail. See Info as below.
        Return values:
           a cursor object of query
        Exceptions:
           pysequoiadb.error.SDBBaseError
        Info:
           query flags:
           QUERY_FLG_WITH_RETURNDATA : Force to use specified hint to query, if database have no index assigned by the hint, fail to query
           QUERY_FLG_PARALLED        : Enable parallel sub query, each sub query will finish scanning different part of the data
           QUERY_FLG_FORCE_HINT      : In general, query won't return data until cursor gets from database, when add this flag, return data in query response, it will be more high-performance
           QUERY_PREPARE_MORE        : Enable prepare more data when query
        """

        bson_condition = None
        bson_selector = None
        bson_order_by = None
        bson_hint = None

        num_to_skip = 0
        num_to_return = -1
        flags = 0

        if "condition" in kwargs:
            if not isinstance(kwargs.get("condition"), dict):
                raise SDBTypeError("condition must be an instance of dict")
            bson_condition = bson.BSON.encode(kwargs.get("condition"))
        if "selector" in kwargs:
            if not isinstance(kwargs.get("selector"), dict):
                raise SDBTypeError("selector must be an instance of dict")
            bson_selector = bson.BSON.encode(kwargs.get("selector"))
        if "order_by" in kwargs:
            if not isinstance(kwargs.get("order_by"), dict):
                raise SDBTypeError("order_by must be an instance of dict")
            bson_order_by = bson.BSON.encode(kwargs.get("order_by"))
        if "hint" in kwargs:
            if not isinstance(kwargs.get("hint"), dict):
                raise SDBTypeError("hint must be an instance of dict")
            bson_hint = bson.BSON.encode(kwargs.get("hint"))
        if "num_to_skip" in kwargs:
            if not isinstance(kwargs.get("num_to_skip"), long_type):
                raise SDBTypeError("num_to_skip must be an instance of long")
            else:
                num_to_skip = kwargs.get("num_to_skip")
        if "num_to_return" in kwargs:
            if not isinstance(kwargs.get("num_to_return"), long_type):
                raise SDBTypeError("num_to_return must be an instance of long")
            else:
                num_to_return = kwargs.get("num_to_return")
        if "flags" in kwargs:
            if not isinstance(kwargs.get("flags"), int):
                raise SDBTypeError("flags must be an instance of int")
            else:
                flags = kwargs.get("flags")

        try:
            result = cursor()
            rc = sdb.cl_query(self._cl, result._cursor,
                              bson_condition, bson_selector,
                              bson_order_by, bson_hint,
                              num_to_skip, num_to_return, flags)
            raise_if_error(rc, "Failed to query")
        except SDBBaseError:
            del result
            result = None
            raise

        return result

    def query_and_update(self, update, **kwargs):
        """Get the matching documents in current collection and update.

        Parameters:
           Name            Type     Info:
           update          dict     The update rule, can't be None.
           **kwargs                 Useful options are below
           - condition     dict     The matching rule, update all the
                                            documents if not provided.
           - selector      dict     The selective rule, return the whole
                                            document if not provided.
           - order_by      dict     The ordered rule, result set is unordered
                                            if not provided.
           - hint          dict     The hint, automatically match the optimal
                                            hint if not provided.
           - num_to_skip   long     Skip the first numToSkip documents,
                                            default is 0L.
           - num_to_return long     Only return numToReturn documents,
                                            default is -1L for returning
                                            all results.
           - flags         int      The query flags, default to be 0. Please see
                                            the definition of follow flags for
                                            more detail. See Info as below.
           - return_new    bool     When True, returns the updated document rather than the original

        Return values:
           a cursor object of query
        Exceptions:
           pysequoiadb.error.SDBBaseError
        Info:
           query flags:
           QUERY_FLG_WITH_RETURNDATA            : Force to use specified hint to query, if database have
                                                        no index assigned by the hint, fail to query
           QUERY_FLG_PARALLED                   : Enable parallel sub query, each sub query will finish scanning
                                                        different part of the data
           QUERY_FLG_FORCE_HINT                 : In general, query won't return data until cursor gets from
                                                        database, when add this flag, return data in query response,
                                                        it will be more high-performance
           QUERY_FLG_KEEP_SHARDINGKEY_IN_UPDATE : The sharding key in update rule is not filtered, when executing
                                                        queryAndUpdate.
        """

        bson_condition = None
        bson_selector = None
        bson_order_by = None
        bson_hint = None
        bson_update = None

        if update is not None:
            if not isinstance(update, dict):
                raise SDBTypeError("update must be an instance of dict")
            bson_update = bson.BSON.encode(update)
        else:
            raise SDBTypeError("update can't be None")

        condition = kwargs.get('condition')
        if condition is not None:
            if not isinstance(condition, dict):
                raise SDBTypeError("condition must be an instance of dict")
            bson_condition = bson.BSON.encode(condition)

        selector = kwargs.get('selector')
        if selector is not None:
            if not isinstance(selector, dict):
                raise SDBTypeError("selector must be an instance of dict")
            bson_selector = bson.BSON.encode(selector)

        order_by = kwargs.get('order_by')
        if order_by is not None:
            if not isinstance(order_by, dict):
                raise SDBTypeError("order_by must be an instance of dict")
            bson_order_by = bson.BSON.encode(order_by)

        hint = kwargs.get('hint')
        if hint is not None:
            if not isinstance(hint, dict):
                raise SDBTypeError("hint must be an instance of dict")
            bson_hint = bson.BSON.encode(hint)

        num_to_skip = 0
        if kwargs.get('num_to_skip') is not None:
            if not isinstance(kwargs.get('num_to_skip'), int):
                raise SDBTypeError("num_to_skip must be an instance of int")
            num_to_skip = kwargs.get('num_to_skip')

        num_to_return = -1
        if kwargs.get('num_to_return') != None:
            if not isinstance(kwargs.get('num_to_return'), int):
                raise SDBTypeError("num_to_return must be an instance of int")
            num_to_return = kwargs.get('num_to_return')

        return_new = False
        if kwargs.get('return_new') != None:
            if not isinstance(kwargs.get('return_new'), bool):
                raise SDBTypeError("return_new must be an instance of bool")
            return_new = kwargs.get('return_new')

        flags = 0
        if "flags" in kwargs:
            if not isinstance(kwargs.get("flags"), int):
                raise SDBTypeError("flags must be an instance of int")
            else:
                flags = kwargs.get("flags")

        try:
            result = cursor()
            rc = sdb.cl_query_and_update(self._cl, result._cursor,
                                         bson_condition, bson_selector,
                                         bson_order_by, bson_hint,
                                         num_to_skip, num_to_return, return_new, flags, bson_update)
            raise_if_error(rc, "Failed to query")
        except SDBBaseError:
            del result
            result = None
            raise

        return result

    def query_and_remove(self, **kwargs):
        """Get the matching documents in current collection and remove.

        Parameters:
           Name            Type     Info:
           **kwargs                 Useful options are below
           - condition     dict     The matching rule, update all the
                                            documents if not provided.
           - selector      dict     The selective rule, return the whole
                                            document if not provided.
           - order_by      dict     The ordered rule, result set is unordered
                                            if not provided.
           - hint          dict     The hint, automatically match the optimal
                                            hint if not provided.
           - num_to_skip   long     Skip the first numToSkip documents,
                                            default is 0L.
           - num_to_return long     Only return numToReturn documents,
                                            default is -1L for returning
                                            all results.
           - flags         int      The query flags, default to be 0. Please see
                                            the definition of follow flags for
                                            more detail. See Info as below.
        Return values:
           a cursor object of query
        Exceptions:
           pysequoiadb.error.SDBBaseError
        Info:
           query flags:
           QUERY_FLG_WITH_RETURNDATA : Force to use specified hint to query, if database have no index assigned by the hint, fail to query
           QUERY_FLG_PARALLED        : Enable parallel sub query, each sub query will finish scanning different part of the data
           QUERY_FLG_FORCE_HINT      : In general, query won't return data until cursor gets from database, when add this flag, return data in query response, it will be more high-performance
        """

        bson_condition = None
        bson_selector = None
        bson_order_by = None
        bson_hint = None

        condition = kwargs.get('condition')
        if condition is not None:
            if not isinstance(condition, dict):
                raise SDBTypeError("condition must be an instance of dict")
            bson_condition = bson.BSON.encode(condition)

        selector = kwargs.get('selector')
        if selector is not None:
            if not isinstance(selector, dict):
                raise SDBTypeError("selector must be an instance of dict")
            bson_selector = bson.BSON.encode(selector)

        order_by = kwargs.get('order_by')
        if order_by is not None:
            if not isinstance(order_by, dict):
                raise SDBTypeError("order_by must be an instance of dict")
            bson_order_by = bson.BSON.encode(order_by)

        hint = kwargs.get('hint')
        if hint is not None:
            if not isinstance(hint, dict):
                raise SDBTypeError("hint must be an instance of dict")
            bson_hint = bson.BSON.encode(hint)

        num_to_skip = 0
        if kwargs.get('num_to_skip') is not None:
            if not isinstance(kwargs.get('num_to_skip'), int):
                raise SDBTypeError("num_to_skip must be an instance of int")
            num_to_skip = kwargs.get('num_to_skip')

        num_to_return = -1
        if kwargs.get('num_to_return') != None:
            if not isinstance(kwargs.get('num_to_return'), int):
                raise SDBTypeError("num_to_return must be an instance of int")
            num_to_return = kwargs.get('num_to_return')

        flags = 0
        if kwargs.get('flags') != None:
            if kwargs.get('flags') not in (0, QUERY_FLG_WITH_RETURNDATA,
                                           QUERY_FLG_PARALLED,
                                           QUERY_FLG_FORCE_HINT):
                raise SDBTypeError("invalid flags value")

        try:
            result = cursor()
            rc = sdb.cl_query_and_remove(self._cl, result._cursor,
                                         bson_condition, bson_selector,
                                         bson_order_by, bson_hint,
                                         num_to_skip, num_to_return, flags)
            raise_if_error(rc, "Failed to query")
        except SDBBaseError:
            del result
            result = None
            raise

        return result

    def create_index(self, index_def, idx_name, is_unique=False, is_enforced=False, buffer_size=64):
        """Create an index in current collection.

        Parameters:
           Name         Type  Info:
           index_def    dict  The dict object of index element.
                                    e.g. {'name':1, 'age':-1}
           idx_name     str   The index name.
           is_unique    bool  Whether the index elements are unique or not.
           is_enforced  bool  Whether the index is enforced unique This
                                    element is meaningful when isUnique is set to
                                    true.
           buffer_size  int   The size of sort buffer used when creating index,
                                    the unit is MB, zero means don't use sort buffer
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(index_def, dict):
            raise SDBTypeError("index definition must be an instance of dict")
        if not isinstance(idx_name, str_type):
            raise SDBTypeError("index name must be an instance of str_type")
        if not isinstance(is_unique, bool):
            raise SDBTypeError("is_unique must be an instance of bool")
        if not isinstance(is_enforced, bool):
            raise SDBTypeError("is_enforced must be an instance of bool")
        if not isinstance(buffer_size, int):
            raise SDBTypeError("is_enforced must be an instance of bool")

        unique = 0
        enforce = 0
        bson_index_def = bson.BSON.encode(index_def)

        if is_unique:
            unique = 1
        if is_enforced:
            enforced = 1

        rc = sdb.cl_create_index(self._cl, bson_index_def, idx_name,
                                 is_unique, is_enforced, buffer_size)
        raise_if_error(rc, "Failed to create index")

    def get_indexes(self, idx_name=None):
        """Get all of or one of the indexes in current collection.

        Parameters:
           Name         Type  Info:
           idx_name     str   The index name, returns all of the indexes
                                    if this parameter is None.
        Return values:
           a cursor object of result
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if idx_name is not None and not isinstance(idx_name, str_type):
            raise SDBTypeError("index name must be an instance of str_type")
        if idx_name is None:
            idx_name = ""

        try:
            result = cursor()
            rc = sdb.cl_get_index(self._cl, result._cursor, idx_name)
            raise_if_error(rc, "Failed to get indexes")
        except SDBBaseError:
            del result
            result = None
            raise

        return result

    def drop_index(self, idx_name):
        """The index name.

        Parameters:
           Name         Type  Info:
           idx_name     str   The index name.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(idx_name, str_type):
            raise SDBTypeError("index name must be an instance of str_type")

        rc = sdb.cl_drop_index(self._cl, idx_name)
        raise_if_error(rc, "Failed to drop index")

    def get_collection_name(self):
        """Get the name of current collection.

        Return values:
           The name of current collection
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        rc, cl_name = sdb.cl_get_collection_name(self._cl)
        raise_if_error(rc, "Failed to get collection name")
        return cl_name

    def get_cs_name(self):
        """Get the name of current collection space.

        Return values:
           The name of current collection space
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        rc, cs_name = sdb.cl_get_collection_space_name(self._cl)
        raise_if_error(rc, "Failed to get collection space name")
        return cs_name

    def get_full_name(self):
        """Get the full name of specified collection in current collection space.

        Return values:
           The full name of current collection
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        rc, full_name = sdb.cl_get_full_name(self._cl)
        raise_if_error(rc, "Failed to get full name")
        return full_name

    def aggregate(self, aggregate_options):
        """Execute aggregate operation in specified collection.

        Parameters:
           Name               Type       Info:
           aggregate_options  list/tuple The array of dict objects.
                                               bson.SON may need if the element is
                                               order-sensitive.
                                               eg.
                                               {'$sort':bson.SON([("name",-1), ("age":1)])}
                                               it will be ordered descending by 'name'
                                               first, and be ordered ascending by 'age'
        Return values:
           a cursor object of result
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(aggregate_options, list):
            raise SDBTypeError("aggregate options must be an instance of list")

        container = []
        for option in aggregate_options:
            if not isinstance(option, dict):
                raise SDBTypeError("options must be an instance of dict")
            bson_option = bson.BSON.encode(option)
            container.append(bson_option)

        result = cursor()
        try:
            rc = sdb.cl_aggregate(self._cl, result._cursor, container)
            raise_if_error(rc, "Failed to aggregate")
        except SDBBaseError:
            del result
            raise

        return result

    def get_query_meta(self, **kwargs):
        """Get the index blocks' or data blocks' infomations for concurrent query.

        Parameters:
           Name              Type     Info:
           **kwargs                   Useful options are below
           - condition       dict     The matching rule, return the whole range
                                            of index blocks if not provided.
                                            eg:{"age":{"$gt":25},"age":{"$lt":75}}.
           - order_by        dict     The ordered rule, result set is unordered
                                            if not provided.bson.SON may need if it is
                                            order-sensitive.
           - hint            dict     One of the indexs in current collection,
                                            using default index to query if not
                                            provided.
                                            eg:{"":"ageIndex"}.
           - num_to_skip     long     Skip the first num_to_skip documents,
                                            default is 0L.
           - num_to_return   long     Only return num_to_return documents,
                                            default is -1L for returning all results.
        Return values:
           a cursor object of query
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        num_to_skip = 0
        num_to_return = -1

        if "num_to_skip" in kwargs:
            if not isinstance(kwargs.get("num_to_skip"), long_type):
                raise SDBTypeError("number to skip must be an instance of long")
            else:
                num_to_skip = kwargs.get("num_to_skip")
        if "num_to_return" in kwargs:
            if not isinstance(kwargs.get("num_to_return"), long_type):
                raise SDBTypeError("number to return must be an instance of long")
            else:
                num_to_return = kwargs.get("num_to_return")

        bson_condition = None
        bson_order_by = None
        bson_hint = None
        if "condition" in kwargs:
            if not isinstance(kwargs.get("condition"), dict):
                raise SDBTypeError("condition must be an instance of dict")
            bson_condition = bson.BSON.encode(kwargs.get("condition"))
        if "order_by" in kwargs:
            if not isinstance(kwargs.get("order_by"), dict):
                raise SDBTypeError("order_by must be an instance of dict")
            bson_order_by = bson.BSON.encode(kwargs.get("order_by"))
        if "hint" in kwargs:
            if not isinstance(kwargs.get("hint"), dict):
                raise SDBTypeError("hint must be an instance of dict")
            bson_hint = bson.BSON.encode(kwargs.get("hint"))

        result = cursor()
        try:
            rc = sdb.cl_get_query_meta(self._cl, result._cursor, bson_condition,
                                       bson_order_by, bson_hint, num_to_skip, num_to_return)
            raise_if_error(rc, "Failed to query meta")
        except SDBBaseError:
            del result
            raise

        return result

    def attach_collection(self, cl_full_name, options):
        """Attach the specified collection.

        Parameters:
           Name            Type  Info:
           subcl_full_name str   The name fo the subcollection.
           options         dict  he low boudary and up boudary
                                       eg: {"LowBound":{a:1},"UpBound":{a:100}}
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(cl_full_name, str_type):
            raise SDBTypeError("full name of subcollection must be \
                          an instance of str_type")
        if not isinstance(options, dict):
            raise SDBTypeError("options must be an instance of str_type")

        bson_options = None
        if options is not None:
            bson_options = bson.BSON.encode(options)

        rc = sdb.cl_attach_collection(self._cl, cl_full_name, bson_options)
        raise_if_error(rc, "Failed to attach collection")

    def detach_collection(self, sub_cl_full_name):
        """Dettach the specified collection.

        Parameters:
           Name            Type  Info:
           subcl_full_name str   The name fo the subcollection.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(sub_cl_full_name, str_type):
            raise SDBTypeError("name of subcollection must be an instance of str_type")

        rc = sdb.cl_detach_collection(self._cl, sub_cl_full_name)
        raise_if_error(rc, "Failed to detach collection")

    def create_lob(self, oid=None):
        """create lob.

        Parameters:
           Name     Type           Info:
           oid      bson.ObjectId  Specified the oid of lob to be created,
                                         if None, the oid is generated automatically
        Return values:
           a lob object
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if oid is None:
            str_id = None
        elif isinstance(oid, bson.ObjectId):
            str_id = str(oid)
        else:
            raise SDBTypeError("oid must be an instance of bson.ObjectId")

        obj = lob()
        try:
            rc = sdb.cl_create_lob(self._cl, obj._handle, str_id)
            raise_if_error(rc, "Failed to create lob")
        except SDBBaseError:
            del obj
            raise

        return obj

    def open_lob(self, oid, mode=LOB_READ):
        """open the specified lob to read or write.

        Parameters:
           Name     Type                 Info:
           oid      str/bson.ObjectId    The specified oid
           mode     int                  The open mode:
                                         lob.LOB_READ
                                         lob.LOB_WRITE
        Return values:
           a lob object
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(oid, bson.ObjectId) and not isinstance(oid, str_type):
            raise SDBTypeError("oid must be bson.ObjectId or string")

        if isinstance(oid, bson.ObjectId):
            str_id = str(oid)
        else:
            str_id = oid
            if len(oid) != 24:
                raise SDBInvalidArgument(SDB_INVALIDARG, "invalid oid: '%s'" % oid)

        if not isinstance(mode, int):
            raise SDBTypeError("mode must be an instance of int")
        if mode != LOB_READ and mode != LOB_WRITE:
            raise SDBTypeError("mode must be lob.LOB_READ or lob.LOB_WRITE")

        obj = lob()
        try:
            rc = sdb.cl_open_lob(self._cl, obj._handle, str_id, mode)
            raise_if_error(rc, "Failed to get specified lob")
        except SDBBaseError:
            del obj
            raise

        return obj

    def get_lob(self, oid):
        """get the specified lob.

        Parameters:
           Name     Type                 Info:
           oid      str/bson.ObjectId    The specified oid
        Return values:
           a lob object
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        return self.open_lob(oid, LOB_READ)

    def remove_lob(self, oid):
        """remove lob.

        Parameters:
           Name     Type                 Info:
           oid      str/bson.ObjectId    The oid of the lob to be remove.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if isinstance(oid, bson.ObjectId):
            str_id = str(oid)
        elif isinstance(oid, str):
            str_id = oid
            if len(oid) != 24:
                raise SDBInvalidArgument(SDB_INVALIDARG, "invalid oid: '%s'" % oid)
        else:
            raise SDBTypeError("oid must be an instance of str or bson.ObjectId")

        rc = sdb.cl_remove_lob(self._cl, str_id)
        raise_if_error(rc, "Failed to remove lob")

    def truncate_lob(self, oid, length):
        """truncate lob.

        Parameters:
           Name     Type                 Info:
           oid      str/bson.ObjectId    The oid of the lob to be truncated.
           length   int/long             The truncate length
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if isinstance(oid, bson.ObjectId):
            str_id = str(oid)
        elif isinstance(oid, str):
            str_id = oid
            if len(oid) != 24:
                raise SDBInvalidArgument(SDB_INVALIDARG, "invalid oid: '%s'" % oid)
        else:
            raise SDBTypeError("oid must be an instance of str or bson.ObjectId")

        if not isinstance(length, (int, long_type)):
            raise SDBTypeError("length must be an instance of int or long")

        rc = sdb.cl_truncate_lob(self._cl, str_id, length)
        raise_if_error(rc, "Failed to truncate lob")

    def list_lobs(self):
        """list all lobs.

        Parameters:
           Name     Type                 Info:

        Return values:
           a cursor object of query
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        result = cursor()
        try:
            rc = sdb.cl_list_lobs(self._cl, result._cursor)
            raise_if_error(rc, "Failed to list lobs")
        except SDBBaseError:
            del result
            raise

        return result

    def query_one(self, **kwargs):
        """Get one matching documents in current collection.

        Parameters:
           Name              Type     Info:
           **kwargs                   Useful options are below
           - condition       dict     The matching rule, update all the
                                            documents if not provided.
           - selected        dict     The selective rule, return the whole
                                            document if not provided.
           - order_by        dict     The ordered rule, result set is unordered
                                            if not provided.
           - hint            dict     The hint, automatically match the optimal
                                            hint if not provided.
           - num_to_skip     long     Skip the first numToSkip documents,
                                            default is 0L.
           - flags           int      The query flags, default to be 0. Please see
                                            the definition of follow flags for
                                            more detail. See Info as below.
        Return values:
           a record of json/dict
        Exceptions:
           pysequoiadb.error.SDBBaseError
        Info:
           query flags:
           QUERY_FLG_WITH_RETURNDATA : Force to use specified hint to query, if database have no index assigned by the hint, fail to query
           QUERY_FLG_PARALLED        : Enable parallel sub query, each sub query will finish scanning different part of the data
           QUERY_FLG_FORCE_HINT      : In general, query won't return data until cursor gets from database, when add this flag, return data in query response, it will be more high-performance
           QUERY_PREPARE_MORE        : Enable prepare more data when query
        """
        bson_condition = None
        bson_selector = None
        bson_order_by = None
        bson_hint = None

        num_to_skip = 0
        flags = 0

        if "condition" in kwargs:
            if not isinstance(kwargs.get("condition"), dict):
                raise SDBTypeError("condition must be an instance of dict")
            bson_condition = bson.BSON.encode(kwargs.get("condition"))
        if "selector" in kwargs:
            if not isinstance(kwargs.get("selector"), dict):
                raise SDBTypeError("selector must be an instance of dict")
            bson_selector = bson.BSON.encode(kwargs.get("selector"))
        if "order_by" in kwargs:
            if not isinstance(kwargs.get("order_by"), dict):
                raise SDBTypeError("order_by must be an instance of dict")
            bson_order_by = bson.BSON.encode(kwargs.get("order_by"))
        if "hint" in kwargs:
            if not isinstance(kwargs.get("hint"), dict):
                raise SDBTypeError("hint must be an instance of dict")
            bson_hint = bson.BSON.encode(kwargs.get("hint"))
        if "num_to_skip" in kwargs:
            if not isinstance(kwargs.get("num_to_skip"), long_type):
                raise SDBTypeError("num_to_skip must be an instance of long")
            else:
                num_to_skip = kwargs.get("num_to_skip")
        if "flags" in kwargs:
            if not isinstance(kwargs.get("flags"), int):
                raise SDBTypeError("flags must be an instance of int")
            else:
                flags = kwargs.get("flags")

        result = cursor()
        try:
            rc = sdb.cl_query(self._cl, result._cursor,
                              bson_condition, bson_selector,
                              bson_order_by, bson_hint,
                              num_to_skip, 1, flags)
            raise_if_error(rc, "Failed to query one")
        except SDBBaseError:
            del result
            raise

        try:
            record = result.next()
        except SDBEndOfCursor:
            record = None
        except SDBBaseError:
            raise

        del result

        return record

    def explain(self, **kwargs):
        """Get the matching documents in current collection.

        Parameters:
           Name              Type     Info:
           **kwargs                   Useful options are below
           - condition       dict     The matching rule, update all the
                                            documents if not provided.
           - selected        dict     The selective rule, return the whole
                                            document if not provided.
           - order_by        dict     The ordered rule, result set is unordered
                                            if not provided.
           - hint            dict     The hint, automatically match the optimal
                                            hint if not provided.
           - num_to_skip     long     Skip the first numToSkip documents,
                                            default is 0L.
           - num_to_return   long     Only return numToReturn documents,
                                            default is -1L for returning
                                            all results.
           - flags           int      The query flags, default to be 0. Please see
                                            the definition of follow flags for
                                            more detail. See Info as below.
           - options         dict
        Return values:
           a cursor object of query
        Exceptions:
           pysequoiadb.error.SDBBaseError
        Info:
           query flags:
           QUERY_FLG_WITH_RETURNDATA : Force to use specified hint to query, if database have no index assigned by the hint, fail to query
           QUERY_FLG_PARALLED        : Enable parallel sub query, each sub query will finish scanning different part of the data
           QUERY_FLG_FORCE_HINT      : In general, query won't return data until cursor gets from database, when add this flag, return data in query response, it will be more high-performance
        """

        bson_condition = None
        bson_selector = None
        bson_order_by = None
        bson_hint = None
        bson_options = None

        num_to_skip = 0
        num_to_return = -1
        flag = 0

        if "condition" in kwargs:
            if not isinstance(kwargs.get("condition"), dict):
                raise SDBTypeError("condition must be an instance of dict")
            bson_condition = bson.BSON.encode(kwargs.get("condition"))
        if "selector" in kwargs:
            if not isinstance(kwargs.get("selector"), dict):
                raise SDBTypeError("selector must be an instance of dict")
            bson_selector = bson.BSON.encode(kwargs.get("selector"))
        if "order_by" in kwargs:
            if not isinstance(kwargs.get("order_by"), dict):
                raise SDBTypeError("order_by must be an instance of dict")
            bson_order_by = bson.BSON.encode(kwargs.get("order_by"))
        if "hint" in kwargs:
            if not isinstance(kwargs.get("hint"), dict):
                raise SDBTypeError("hint must be an instance of dict")
            bson_hint = bson.BSON.encode(kwargs.get("hint"))
        if "num_to_skip" in kwargs:
            if not isinstance(kwargs.get("num_to_skip"), long_type):
                raise SDBTypeError("num_to_skip must be an instance of long")
            else:
                num_to_skip = kwargs.get("num_to_skip")
        if "num_to_return" in kwargs:
            if not isinstance(kwargs.get("num_to_return"), long_type):
                raise SDBTypeError("num_to_return must be an instance of long")
            else:
                num_to_return = kwargs.get("num_to_return")
        if "flag" in kwargs:
            if not isinstance(kwargs.get("flag"), int):
                raise SDBTypeError("flag must be an instance of int")
            else:
                num_to_return = kwargs.get("flag")
        if "options" in kwargs:
            if not isinstance(kwargs.get("options"), dict):
                raise SDBTypeError("options must be an instance of dict")
            bson_options = bson.BSON.encode(kwargs.get("options"))

        result = cursor()
        try:
            rc = sdb.cl_explain(self._cl, result._cursor,
                                bson_condition, bson_selector,
                                bson_order_by, bson_hint,
                                num_to_skip, num_to_return,
                                flag, bson_options)
            raise_if_error(rc, "Failed to explain")
        except SDBBaseError:
            del result
            raise

        return result

    def truncate(self):
        """truncate the collection.

        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        rc = sdb.cl_truncate(self._cl)
        raise_if_error(rc, "Truncate failed")

    def create_id_index(self, options=None):
        """Create the id index.

        Parameters:
           Name         Type     Info:
           options      dict     The configuration options for id index.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """

        if options is None:
            options = {}
        elif not isinstance(options, dict):
            raise SDBTypeError("options must be an instance of dict")

        bson_options = bson.BSON.encode(options)
        rc = sdb.cl_create_id_index(self._cl, bson_options)
        raise_if_error(rc, "Create id index failed")

    def drop_id_index(self):
        """Drop the id index.

        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        rc = sdb.cl_drop_id_index(self._cl)
        raise_if_error(rc, "Drop id index failed")

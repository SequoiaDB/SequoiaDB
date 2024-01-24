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
from pysequoiadb.lob import *
from pysequoiadb.error import (SDBBaseError,
                               SDBTypeError,
                               SDBSystemError,
                               SDBEndOfCursor,
                               SDBInvalidArgument,
                               raise_if_error)
from pysequoiadb.errcode import (SDB_OOM, SDB_INVALIDARG)

QUERY_FLG_FORCE_HINT = 0x00000080
QUERY_FLG_PARALLED = 0x00000100
QUERY_FLG_WITH_RETURNDATA = 0x00000200
QUERY_PREPARE_MORE = 0x00004000
QUERY_FLG_KEEP_SHARDINGKEY_IN_UPDATE = 0x00008000
QUERY_FLG_FOR_UPDATE = 0x00010000
QUERY_FLG_FOR_SHARE = 0x00040000

UPDATE_FLG_KEEP_SHARDINGKEY = QUERY_FLG_KEEP_SHARDINGKEY_IN_UPDATE
UPDATE_FLG_UPDATE_ONE = 0x00000002
UPDATE_FLG_RETURNNUM = 0x00000004

DELETE_FLG_DELETE_ONE = 0x00000002
DELETE_FLG_RETURNNUM = 0x00000004

INSERT_FLG_DEFAULT = 0x00000000
INSERT_FLG_CONTONDUP = 0x00000001
INSERT_FLG_RETURNNUM = 0x00000002
INSERT_FLG_REPLACEONDUP = 0x00000004
INSERT_FLG_CONTONDUP_ID = 0x00000020
INSERT_FLG_REPLACEONDUP_ID = 0x00000040
INSERT_FLG_RETURN_OID = 0x10000000

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

    def bulk_insert(self, flag, records):
        """Insert a bulk of record into current collection.

        Parameters:
           Name        Type       Info:
           flag        int        See Info as below.
           records     list/tuple The list of inserted records.
        Return values:
            A dict object contains the insert details. As follow:
            - InsertedNum    : The number of records successfully inserted, including replaced and ignored records.
            - DuplicatedNum  : The number of records ignored or replaced due to duplicate key conflicts.
            - LastGenerateID : The max value of all auto-increments that the first record inserted contains. The
                               result will include this field if current collection has auto-increments.
            - _id            : ObjectId of the inserted record. The result will include field "_id" if
                               FLG_INSERT_RETURN_OID is used.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        Info:
           The flag to control the behavior of inserting. The value of flag default to be INSERT_FLG_DEFAULT, and it
           can choose the follow values:
             INSERT_FLG_DEFAULT      : While INSERT_FLG_DEFAULT is set, database will stop inserting when the record
                                       hit index key duplicate error.
             INSERT_FLG_CONTONDUP    : If the record hit index key duplicate error, database will skip it.
             INSERT_FLG_RETURN_OID   : Return the value of "_id" field in the record.
             INSERT_FLG_REPLACEONDUP : If the record hit index key duplicate error, database will replace the existing
                                       record by the inserting new record and then go on inserting.
             INSERT_FLG_CONTONDUP_ID :  The flag represent the error of the dup key will be ignored when the dup key is '_id'.
             INSERT_FLG_REPLACEONDUP_ID : The flag represents the error of the dup key will be ignored when the dup key is '_id',
                                          and the original record will be replaced by new record.
        """
        if not isinstance(flag, int):
            raise SDBTypeError("flags must be an instance of int")

        container = []
        for elem in records:
            if not isinstance(elem, dict):
                raise SDBTypeError("record must be an instance of dict")
            record = bson.BSON.encode(elem)
            container.append(record)

        rc, bson_string = sdb.cl_bulk_insert(self._cl, flag, container)
        raise_if_error(rc, "Failed to insert records")
        result, size = bson._bson_to_dict(bson_string, dict, False,
                                          bson.OLD_UUID_SUBTYPE, True)
        return result

    def insert(self, record):
        """Insert a record into current collection.

        Parameters:
           Name      Type    Info:
           record    dict    The inserted record.
        Return values:
           An ObjectId object of record inserted. eg: { '_id': ObjectId('5d5149ade3071dce3692e93b') }
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

    def insert_with_flag(self, record, flag=INSERT_FLG_DEFAULT):
        """Insert a record into current collection.

         Parameters:
            Name      Type    Info:
            record    dict    The inserted record.
            flag      int     See Info as below.
         Return values:
            A dict object contains the insert details. As follow:
            - InsertedNum    : The number of records successfully inserted, including replaced and ignored records.
            - DuplicatedNum  : The number of records ignored or replaced due to duplicate key conflicts.
            - LastGenerateID : The max value of all auto-increments that the inserted record contains. The result
                               will include this field if current collection has auto-increments.
            - _id            : ObjectId of the inserted record. The result will include field "_id" if
                               FLG_INSERT_RETURN_OID is used.
         Exceptions:
            pysequoiadb.error.SDBBaseError
         Info:
           The flag to control the behavior of inserting. The value of flag default to be INSERT_FLG_DEFAULT, and it
           can choose the follow values:
             INSERT_FLG_DEFAULT      : While INSERT_FLG_DEFAULT is set, database will stop inserting when the record
                                       hit index key duplicate error.
             INSERT_FLG_CONTONDUP    : If the record hit index key duplicate error, database will skip it.
             INSERT_FLG_RETURN_OID   : Return the value of "_id" field in the record.
             INSERT_FLG_REPLACEONDUP : If the record hit index key duplicate error, database will replace the existing
                                       record by the inserting new record and then go on inserting.
             INSERT_FLG_CONTONDUP_ID :  The flag represent the error of the dup key will be ignored when the dup key is '_id'.
             INSERT_FLG_REPLACEONDUP_ID : The flag represents the error of the dup key will be ignored when the dup key is '_id',
                                          and the original record will be replaced by new record.
         """
        if not isinstance(record, dict):
            raise SDBTypeError("record must be an instance of dict")
        if not isinstance(flag, int):
            raise SDBTypeError("flags must be an instance of int")

        bson_record = bson.BSON.encode(record)
        rc, bson_string = sdb.cl_insert_with_flag(self._cl, bson_record, flag)
        raise_if_error(rc, "Failed to insert record")
        result, size = bson._bson_to_dict(bson_string, dict, False,
                                          bson.OLD_UUID_SUBTYPE, True)
        return result

    def update(self, rule, **kwargs):
        """Update the matching documents in current collection.

        Parameters:
           Name        Type     Info:
           rule        dict     The updating rule.
           **kwargs             Useful option are below
           - condition dict     The matching rule, match all the documents
                                        if not provided.
           - hint      dict     The hint, automatically match the optimal hint
                                        if not provided.
           - flags     int      See Info as below.
        Return values:
            A dict object contains the update details. As follow:
            - UpdatedNum  : The number of records successfully updated, including records that match but have no
                            data changes.
            - ModifiedNum : The number of records successfully updated with data changes.
            - InsertedNum : The number of records successfully inserted.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        Info:
           The update flags, default to be 0, it can choose the follow values:
             UPDATE_FLG_KEEP_SHARDINGKEY : The sharding key in update rule is not filtered, when executing
                                                   update or upsert.
             UPDATE_FLG_UPDATE_ONE       : The flag represent whether to update only one matched record or all
                                                   matched records.
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

        rc, bson_string = sdb.cl_update(self._cl, bson_rule, bson_condition, bson_hint, flags)
        raise_if_error(rc, "Failed to update")
        result, size = bson._bson_to_dict(bson_string, dict, False,
                                          bson.OLD_UUID_SUBTYPE, True)
        return result

    def upsert(self, rule, **kwargs):
        """Update the matching documents in current collection, insert if
           no matching.

        Parameters:
           Name          Type  Info:
           rule          dict  The updating rule.
           **kwargs            Useful options are below
           - condition   dict  The matching rule, match all the documents
                                       if not provided.
           - hint        dict  The hint, automatically match the optimal hint
                                       if not provided.
           - setOnInsert dict  The setOnInsert assigns the specified values
                                       to the fields when insert.
           - flags       int   See Info as below.
        Return values:
            A dict object contains the upsert details. As follow:
            - UpdatedNum  : The number of records successfully updated, including records that match but have no
                            data changes.
            - ModifiedNum : The number of records successfully updated with data changes.
            - InsertedNum : The number of records successfully inserted.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        Info:
           The update flags, default to be 0, it can choose the follow values:
             UPDATE_FLG_KEEP_SHARDINGKEY : The sharding key in update rule is not filtered, when executing
                                                   update or upsert.
             UPDATE_FLG_UPDATE_ONE       : The flag represent whether to update only one matched record or all
                                                   matched records.
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

        rc,bson_string = sdb.cl_upsert(self._cl, bson_rule, bson_condition, bson_hint,
                           bson_setOnInsert, flags)
        raise_if_error(rc, "Failed to update")
        result, size = bson._bson_to_dict(bson_string, dict, False,
                                          bson.OLD_UUID_SUBTYPE, True)
        return result

    def save(self, doc):
        """Upsert the record using the main key '_id' of the record.

        Parameters:
           Name          Type  Info:
           doc           dict  The updating rule.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        Note:
           It won't work to update the "ShardingKey" field, but the other fields
                 take effect.
        Deprecated:
           This function is deprecated, use upsert() or insert_with_flag() instead of it.
        """
        if not isinstance(doc, dict):
            raise SDBTypeError("rule must be an instance of dict")

        if "_id" in doc:
            oid = doc.get("_id")
            return self.upsert({"$set": doc}, condition={"_id": oid})
        else:
            return self.insert_with_flag(doc)

    def delete(self, **kwargs):
        """Delete the matching documents in current collection.

        Parameters:
           Name        Type  Info:
           **kwargs          Useful options are below
           - condition dict  The matching rule, match all the documents
                                     if not provided.
           - hint      dict  The hint, automatically match the optimal hint
                                     if not provided.
           - flags     int   See Info as below.
        Return values:
            A dict object contains the deletion details. As follow:
            - DeletedNum : The number of records successfully deleted.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        Info:
           The delete flags, default to be 0, it can choose the follow values:
             DELETE_FLG_DELETE_ONE : The flag represent whether to delete only one matched record
                                             or all matched records.
        """
        bson_condition = None
        bson_hint = None
        flags = 0

        if "condition" in kwargs:
            if not isinstance(kwargs.get("condition"), dict):
                raise SDBTypeError("condition must be an instance of dict")
            bson_condition = bson.BSON.encode(kwargs.get("condition"))
        if "hint" in kwargs:
            if not isinstance(kwargs.get("hint"), dict):
                raise SDBTypeError("hint must be an instance of dict")
            bson_hint = bson.BSON.encode(kwargs.get("hint"))

        if "flags" in kwargs:
            if not isinstance(kwargs.get("flags"), int):
               raise SDBTypeError("flags must be an instance of int")
            else:
               flags = kwargs.get("flags")

        rc, bson_string = sdb.cl_delete(self._cl, bson_condition, bson_hint, flags)
        raise_if_error(rc, "Failed to delete")
        result, size = bson._bson_to_dict(bson_string, dict, False,
                                          bson.OLD_UUID_SUBTYPE, True)
        return result

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
           QUERY_FLG_FORCE_HINT      : Force to use specified hint to query, if database have no index assigned by the hint, fail to query
           QUERY_FLG_PARALLED        : Enable parallel sub query, each sub query will finish scanning different part of the data
           QUERY_FLG_WITH_RETURNDATA : In general, query won't return data until cursor gets from database, when add this flag, return data in query response, it will be more high-performance
           QUERY_PREPARE_MORE        : Enable prepare more data when query
           QUERY_FLG_FOR_UPDATE      : Acquire U lock on the records that are read. When the session is in
                                       transaction and setting this flag, the transaction lock will not released
                                       until the transaction is committed or rollback. When the session is not
                                       in transaction, the flag does not work.
           QUERY_FLG_FOR_SHARE       : Acquire S lock on the records that are read. When the session is in
                                       transaction and setting this flag, the transaction lock will not released
                                       until the transaction is committed or rollback. When the session is not
                                       in transaction, the flag does not work.
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
           QUERY_FLG_FORCE_HINT                 : Force to use specified hint to query, if database have
                                                        no index assigned by the hint, fail to query
           QUERY_FLG_PARALLED                   : Enable parallel sub query, each sub query will finish scanning
                                                        different part of the data
           QUERY_FLG_WITH_RETURNDATA            : In general, query won't return data until cursor gets from
                                                        database, when add this flag, return data in query response,
                                                        it will be more high-performance
           QUERY_FLG_KEEP_SHARDINGKEY_IN_UPDATE : The sharding key in update rule is not filtered, when executing
                                                        queryAndUpdate.
           QUERY_FLG_FOR_UPDATE      : Acquire U lock on the records that are read. When the session is in
                                       transaction and setting this flag, the transaction lock will not released
                                       until the transaction is committed or rollback. When the session is not
                                       in transaction, the flag does not work.
           QUERY_FLG_FOR_SHARE       : Acquire S lock on the records that are read. When the session is in
                                       transaction and setting this flag, the transaction lock will not released
                                       until the transaction is committed or rollback. When the session is not
                                       in transaction, the flag does not work.
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
           QUERY_FLG_FORCE_HINT      : Force to use specified hint to query, if database have no index assigned by the hint, fail to query
           QUERY_FLG_PARALLED        : Enable parallel sub query, each sub query will finish scanning different part of the data
           QUERY_FLG_WITH_RETURNDATA : In general, query won't return data until cursor gets from database, when add this flag, return data in query response, it will be more high-performance
           QUERY_FLG_FOR_UPDATE      : Acquire U lock on the records that are read. When the session is in
                                       transaction and setting this flag, the transaction lock will not released
                                       until the transaction is committed or rollback. When the session is not
                                       in transaction, the flag does not work.
           QUERY_FLG_FOR_SHARE       : Acquire S lock on the records that are read. When the session is in
                                       transaction and setting this flag, the transaction lock will not released
                                       until the transaction is committed or rollback. When the session is not
                                       in transaction, the flag does not work.
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
                                           QUERY_FLG_FORCE_HINT,
                                           QUERY_FLG_FOR_UPDATE,
                                           QUERY_FLG_FOR_SHARE):
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
                                      eg. {'name':1, 'age':-1}
           idx_name     str   The index name.
           is_unique    bool  Whether the index elements are unique or not.
           is_enforced  bool  Whether the index is enforced unique, this element
                                      is meaningful when isUnique is set to true.
           buffer_size  int   The size of sort buffer used when creating index,
                                      the unit is MB, zero means don't use sort buffer
        Exceptions:
           pysequoiadb.error.SDBBaseError

        """
        if not isinstance(is_unique, bool):
            raise SDBTypeError("is_unique must be an instance of bool")
        if not isinstance(is_enforced, bool):
            raise SDBTypeError("is_enforced must be an instance of bool")
        if not isinstance(buffer_size, int):
            raise SDBTypeError("buffer_size must be an instance of int")

        option = {"Unique":is_unique, "Enforced":is_enforced, "NotNull":False, "SortBufferSize":buffer_size}
        self.create_index_with_option(index_def, idx_name, option)


    def create_index_with_option(self, index_def, idx_name, option=None):
        """Create an index in current collection.

        Parameters:
           Name         Type     Info:
           index_def    dict     The dict object of index element.
                                         eg. {'name':1, 'age':-1}
           idx_name     str      The index name.
           option      dict      The configuration option for index. visit this url:
                                         "http://doc.sequoiadb.com/cn/sequoiadb-cat_id-1432190830-edition_id-@SDB_SYMBOL_VERSION"
                                         to get more details.
        Exceptions:
           pysequoiadb.error.SDBBaseError

        """
        if not isinstance(index_def, dict):
            raise SDBTypeError("index definition must be an instance of dict")
        if not isinstance(idx_name, str_type):
            raise SDBTypeError("index name must be an instance of str_type")
        if option is None:
            option = {}
        elif not isinstance(option, dict):
            raise SDBTypeError("option must be an instance of dict")

        bson_index_def = bson.BSON.encode(index_def)
        bson_option = bson.BSON.encode(option)
        rc = sdb.cl_create_index(self._cl, bson_index_def, idx_name, bson_option)

        raise_if_error(rc, "Failed to create index")

    def get_indexes(self, idx_name=None):
        """Get all of or one of the indexes in current collection.

        Parameters:
           Name         Type  Info:
           idx_name     str   The index name, returns all of the indexes if this parameter is None.
                              Deprecated, use get_index_info to get specific index.
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

    def get_index_info(self, idx_name):
        """Get the information of specified index in current collection.

        Parameters:
           Name         Type  Info:
           idx_name     str   The index name.
        Return values:
           a record of json/dict
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if idx_name is None or not isinstance(idx_name, str_type):
            raise SDBTypeError("index name must be an instance of str_type")

        if idx_name == "":
            return None

        result = cursor()
        try:
            rc = sdb.cl_get_index(self._cl, result._cursor, idx_name)
            raise_if_error(rc, "Failed to get indexes")
        except SDBBaseError:
            del result
            result = None
            raise

        try:
            record = result.next()
        except SDBEndOfCursor:
            record = None
        except SDBBaseError:
            raise
        finally:
            result.close()

        del result

        return record

    def is_index_exist(self, idx_name):
        """Test if the specified index exists or not.

        Parameters:
           Name         Type  Info:
           idx_name     str   The index name.
        Return values:
           bool
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if idx_name is None or not isinstance(idx_name, str_type):
            raise SDBTypeError("index name must be an instance of str_type")

        try:
            result = self.get_index_info(idx_name)
        except SDBBaseError:
            return False

        if result is not None:
            return True
        else:
            return False

    def drop_index(self, idx_name):
        """Removed the named index of current collection.

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

    def get_index_stat(self, idx_name):
        """Get the statistics of the index.

        Parameters:
           Name         Type  Info:
           idx_name     str   The index name.
        Return values:
           a dict object of result
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(idx_name, str_type):
            raise SDBTypeError("index name must be an instance of str_type")

        rc, result = sdb.cl_get_index_stat(self._cl, idx_name)
        raise_if_error(rc, "Failed to get index statistics")
        record, size = bson._bson_to_dict(result, dict, False,
                                          bson.OLD_UUID_SUBTYPE, True)
        return record

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
           subcl_full_name str   The name of the subcollection.
           options         dict  The low boudary and up boudary
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
        """Create a lob.

        Parameters:
           Name     Type           Info:
           oid      bson.ObjectId  Specified the oid of lob to be created,
                                           if None, the oid is generated automatically
        Return values:
           A lob object.
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

    def create_lob_id(self, timestamp=None):
        """ Create a lob id.

        Parameters:
           Name        Type         Info:
           timestamp   str          Used to generate lob id, if be None the timestamp will be generated by server.
                                            format:YYYY-MM-DD-HH.mm.ss. eg: "2019-07-23-18.04.07".
        Return values:
           An ObjectId object of lob.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        
        if timestamp is None:
            str_date = None
        elif isinstance(timestamp, str_type):
            str_date = timestamp
        else:
            raise SDBTypeError("timestamp must be an instance of str")

        rc, id_str = sdb.cl_create_lob_id(self._cl, str_date)
        raise_if_error(rc, "Failed to create lob id")

        oid = bson.ObjectId(id_str)
        return oid



    def open_lob(self, oid, mode=LOB_READ):
        """open the specified lob to read or write.

        Parameters:
           Name     Type                 Info:
           oid      str/bson.ObjectId    The specified oid
           mode     int                  The open mode:
                                         lob.LOB_READ for reading.
                                         lob.LOB_WRITE for writing.
                                         lob.LOB_SHARE_READ for share reading.
                                         lob.LOB_SHARE_READ | lob.LOB_WRITE for both reading and writing.
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
        if not is_read_only_mode(mode) and not has_write_mode(mode):
            raise SDBTypeError("mode is unsupported: " + mode)

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
        return self.open_lob(oid, LOB_SHARE_READ)

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

    def list_lobs(self, **kwargs):
        """list lobs.

        Parameters:
           Name              Type     Info:
           - condition       dict     The matching rule, return all the lob if not provided.
           - selected        dict     The selective rule, return the whole infomation if not provided.
           - order_by        dict     The ordered rule, result set is unordered if not provided.
           - hint            dict     Specified options. eg. {"ListPieces": 1} means get the detail piece info of lobs.
           - num_to_skip     long     Skip the first numToSkip lob, default is 0.
           - num_to_Return   long     Only return numToReturn lob, default is -1 for returning all results.

        Return values:
           a cursor object of query
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """

        bson_condition = None
        bson_selected = None
        bson_order_by = None
        bson_hint = None
        num_to_skip = 0
        num_to_Return = -1

        if "condition" in kwargs:
            if not isinstance(kwargs.get("condition"), dict):
                raise SDBTypeError("condition must be an instance of dict")
            else:
                bson_condition = bson.BSON.encode(kwargs.get("condition"))

        if "selected" in kwargs:
            if not isinstance(kwargs.get("selected"), dict):
                raise SDBTypeError("selected must be an instance of dict")
            else:
                bson_selected = bson.BSON.encode(kwargs.get("selected"))

        if "order_by" in kwargs:
            if not isinstance(kwargs.get("order_by"), dict):
                raise SDBTypeError("order_by must be an instance of dict")
            else:
                bson_order_by = bson.BSON.encode(kwargs.get("order_by"))

        if "hint" in kwargs:
            if not isinstance(kwargs.get("hint"), dict):
                raise SDBTypeError("hint must be an instance of dict")
            else:
                bson_hint = bson.BSON.encode(kwargs.get("hint"))

        if "num_to_skip" in kwargs:
            if not isinstance(kwargs.get("num_to_skip"), int):
                raise SDBTypeError("num_to_skip to skip must be an instance of int")
            else:
                num_to_skip = kwargs.get("num_to_skip")

        if "num_to_Return" in kwargs:
            if not isinstance(kwargs.get("num_to_Return"), int):
                raise SDBTypeError("num_to_Return to skip must be an instance of int")
            else:
                num_to_Return = kwargs.get("num_to_Return")

        result = cursor()
        try:
            rc = sdb.cl_list_lobs(self._cl, result._cursor, bson_condition,
                                  bson_selected, bson_order_by, bson_hint, num_to_skip, num_to_Return)
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
           QUERY_FLG_FORCE_HINT      : Force to use specified hint to query, if database have no index assigned by the hint, fail to query
           QUERY_FLG_PARALLED        : Enable parallel sub query, each sub query will finish scanning different part of the data
           QUERY_FLG_WITH_RETURNDATA : In general, query won't return data until cursor gets from database, when add this flag, return data in query response, it will be more high-performance
           QUERY_PREPARE_MORE        : Enable prepare more data when query
           QUERY_FLG_FOR_UPDATE      : Acquire U lock on the records that are read. When the session is in
                                       transaction and setting this flag, the transaction lock will not released
                                       until the transaction is committed or rollback. When the session is not
                                       in transaction, the flag does not work.
           QUERY_FLG_FOR_SHARE       : Acquire S lock on the records that are read. When the session is in
                                       transaction and setting this flag, the transaction lock will not released
                                       until the transaction is committed or rollback. When the session is not
                                       in transaction, the flag does not work.
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
        finally:
            result.close()

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
           QUERY_FLG_FORCE_HINT      : Force to use specified hint to query, if database have no index assigned by the hint, fail to query
           QUERY_FLG_PARALLED        : Enable parallel sub query, each sub query will finish scanning different part of the data
           QUERY_FLG_WITH_RETURNDATA : In general, query won't return data until cursor gets from database, when add this flag, return data in query response, it will be more high-performance
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

    def alter(self, options):
        """Alter the collection.
        Parameters:
           Name         Type     Info:
           options      dict     The options are as following:
                                 ReplSize            : Assign how many replica nodes need to be synchronized when a write
                                                     request (insert, update, etc) is executed, default is 1
                                 ShardingKey         : Assign the sharding key, foramt: { ShardingKey: { <key name>: <1/-1>} },
                                                     1 indicates positive order, -1 indicates reverse order.
                                                     e.g. { ShardingKey: { age: 1 } }
                                 ShardingType        : Assign the sharding type, default is "hash"
                                 Partition           : The number of partition, it is valid when ShardingType is "hash",
                                                     the range is [2^3,2^20], default is 4096
                                 AutoSplit           : Whether to enable the automatic partitioning, it is valid when
                                                     ShardingType is "hash", defalut is false
                                 EnsureShardingIndex : Whether to build sharding index, default is true
                                 Compressed          : Whether to enable data compression, default is true
                                 CompressionType     : The compression type of data, could be "snappy" or "lzw", default is "lzw"
                                 StrictDataMode      : Whether to enable strict date mode in numeric operations, default is false
                                 AutoIncrement       : Assign attributes of an autoincrement field or batch autoincrement fields
                                                     e.g. { AutoIncrement : { Field : "a", MaxValue : 2000 } },
                                                     { AutoIncrement : [ { Field : "a", MaxValue : 2000}, { Field : "a", MaxValue : 4000 } ] }
                                 AutoIndexId         : Whether to build "$id" index, default is true
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(options, dict):
            raise SDBTypeError("options must be an instance of dict")
        bson_options = bson.BSON.encode(options)
        rc = sdb.cl_alter(self._cl, bson_options)
        raise_if_error(rc, "Alter collection failed")

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

    def enable_sharding(self, options):
        """Alter the collection to enable sharding.
        Parameters:
           Name         Type     Info:
           options      dict     The options to alter
                                 ShardingKey  : Assign the sharding key
                                 ShardingType : Assign the sharding type
                                 Partition    : When the ShardingType is "hash", need to assign Partition, it's the bucket number for hash, the range is [2^3,2^20]
                                 EnsureShardingIndex : Assign to true to build sharding index
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(options, dict):
            raise SDBTypeError("options must be an instance of dict")
        bson_options = bson.BSON.encode(options)
        rc = sdb.cl_enable_sharding(self._cl, bson_options)
        raise_if_error(rc, "Enable sharding failed")

    def disable_sharding(self):
        """Alter the collection to disable sharding.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        rc = sdb.cl_disable_sharding(self._cl)
        raise_if_error(rc, "Disable sharding failed")

    def enable_compression(self, options):
        """Alter the collection to enable compression.
        Parameters:
           Name         Type     Info:
           options      dict     The options to alter
                                 CompressionType : The compression type of data, could be "snappy" or "lzw"
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(options, dict):
            raise SDBTypeError("options must be an instance of dict")
        bson_options = bson.BSON.encode(options)
        rc = sdb.cl_enable_compression(self._cl, bson_options)
        raise_if_error(rc, "Enable compression failed")

    def disable_compression(self):
        """Alter the collection to disable compression.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        rc = sdb.cl_disable_compression(self._cl)
        raise_if_error(rc, "Disable compression failed")

    def set_attributes(self, options):
        """Alter the collection.
        Parameters:
           Name         Type     Info:
           options      dict     The options to alter
                                 ReplSize     : Assign how many replica nodes need to be synchronized when a write request(insert, update, etc) is executed
                                 ShardingKey  : Assign the sharding key
                                 ShardingType : Assign the sharding type
                                 Partition    : When the ShardingType is "hash", need to assign Partition, it's the bucket number for hash, the range is [2^3,2^20]
                                 CompressionType : The compression type of data, could be "snappy" or "lzw"
                                 EnsureShardingIndex : Assign to true to build sharding index
                                 StrictDataMode : Using strict date mode in numeric operations or not
                                 AutoIncrement : Assign attributes of an autoincrement field or batch autoincrement fields. e.g { "AutoIncrement": { "Field": "studentID", "StartValue": 1 } }
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(options, dict):
            raise SDBTypeError("options must be an instance of dict")
        bson_options = bson.BSON.encode(options)
        rc = sdb.cl_set_attributes(self._cl, bson_options)
        raise_if_error(rc, "Set attributes failed")

    def create_autoincrement(self, options):
        """Create autoincrement field on collection.

        Parameters:
           Name      Type           Info:
           options   dict           The options for create_autoincrement. e.g. { Field: "a", MaxValue:2000 }.
           options   list/tuple     It can also be a list/tuple of such dict. e.g. [ { Field: "a", MaxValue:2000 }, { Field: "b", MaxValue:5000 } ]
                                    Available options are as below:
                                    Field          : The name of autoincrement field
                                    StartValue     : The start value of autoincrement field
                                    MinValue       : The minimum value of autoincrement field
                                    MaxValue       : The maxmun value of autoincrement field
                                    Increment      : The increment value of autoincrement field
                                    CacheSize      : The cache size of autoincrement field
                                    AcquireSize    : The acquire size of autoincrement field
                                    Cycled         : The cycled flag of autoincrement field
                                    Generated      : The generated mode of autoincrement field
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not ( isinstance(options, dict) or isinstance(options, list) or isinstance(options, tuple) ):
            raise SDBTypeError("options must be an instance of dict, list or tuple")

        container = []
        if isinstance(options, list) or isinstance(options, tuple):
            for elem in options:
                if not isinstance(elem, dict):
                    raise SDBTypeError("item in list/tuple must be an instance of dict")
                option = bson.BSON.encode(elem)
                container.append(option)
        else:
            option = bson.BSON.encode(options)
            container.append(option)

        rc = sdb.cl_create_autoincrement(self._cl, container)
        raise_if_error(rc, "Create autoincrement failed")

    def drop_autoincrement(self, names):
        """Drop autoincrement field on collection.

        Parameters:
           Name      Type           Info:
           names     str            The name(s) of autoincrement field. e.g. "a"
           names     list/tuple     It can also be a list/tuple of such str. e.g. [ "a", "b" ]
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not ( isinstance(names, str) or isinstance(names, list) or isinstance(names, tuple) ):
            raise SDBTypeError("names must be an instance of str, list or tuple")

        container = []
        if isinstance(names, list) or isinstance(names, tuple):
            for elem in names:
                if not isinstance(elem, str):
                    raise SDBTypeError("item in list/tuple must be an instance of str")
                container.append(elem)
        else:
            container.append(names)

        rc = sdb.cl_drop_autoincrement(self._cl, container)
        raise_if_error(rc, "Drop autoincrement failed")

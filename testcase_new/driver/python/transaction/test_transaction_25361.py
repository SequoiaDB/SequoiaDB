# -*- coding: utf-8 -*-
# @decription: seqDB-25361:事务中指定SDB_FLG_QUERY_FOR_SHARE查询
# @decription: seqDB-25362:不开起事务使用SDB_FLG_QUERY_FOR_SHARE读数据
# @author:     liuli
# @createTime: 2022.02.14

import unittest
from lib import testlib
from pysequoiadb.error import (SDBBaseError, SDBEndOfCursor)
from pysequoiadb.client import SDB_SNAP_TRANSACTIONS_CURRENT
from pysequoiadb.collection import QUERY_FLG_FOR_SHARE

cs_neme = "cs_25361"


class TestQueryWithFlags25361(testlib.SdbTestBase):
    def setUp(self):
        testlib.drop_cs(self.db, cs_neme, ignore_not_exist=True)

    def test_query_25361(self):
        cl_name = "cl_25361"
        index_name = "index_25361"

        dbcs = self.db.create_collection_space(cs_neme)
        dbcl = dbcs.create_collection(cl_name)

        docs = []
        for i in range(0, 20):
            docs.append({"a": i, "b": "test" + str(i)})
        dbcl.bulk_insert(0, docs)
        dbcl.create_index({"a": 1}, index_name)

        # 修改会话属性
        sessionAttr = {"TransIsolation": 0, "TransMaxLockNum": 10}
        self.db.set_session_attri(sessionAttr)

        # 开启事务后查询10条数据
        self.db.transaction_begin()
        condition = {"a": {"$lt": 10}}
        hint = {"": index_name}
        cursor = dbcl.query(condition=condition, hint=hint)
        while True:
            try:
                cursor.next()
            except BaseException as e:
                break
        cursor.close()
        # 校验没有记录锁为S锁
        lock_count = self.get_cl_lock_count(self.db, "S")
        self.assertEqual(lock_count, 0)

        # 指定flags为SDB_FLG_QUERY_FOR_SHARE查询10条数据
        cursor = dbcl.query(condition=condition, hint=hint, flags=QUERY_FLG_FOR_SHARE)
        while True:
            try:
                cursor.next()
            except BaseException as e:
                break
        cursor.close()
        # 校验记录锁中S锁数量为10，集合锁为IS，没有发生锁升级
        lock_count = self.get_cl_lock_count(self.db, "S")
        self.assertEqual(lock_count, 10)
        self.check_is_lock_escalated(self.db, False)
        self.check_cl_lock_type(self.db, "IS")

        # 不指定flags查询后10条数据
        condition = {"a": {"$gte": 10}}
        cursor = dbcl.query(condition=condition, hint=hint)
        while True:
            try:
                cursor.next()
            except BaseException as e:
                break
        cursor.close()
        # 记录锁数量不变，集合锁不变，没有发生锁升级
        lock_count = self.get_cl_lock_count(self.db, "S")
        self.assertEqual(lock_count, 10)
        self.check_is_lock_escalated(self.db, False)
        self.check_cl_lock_type(self.db, "IS")

        # 指定flags为SDB_FLG_QUERY_FOR_SHARE查询后10条数据
        cursor = dbcl.query(condition=condition, hint=hint, flags=QUERY_FLG_FOR_SHARE)
        while True:
            try:
                cursor.next()
            except BaseException as e:
                break
        cursor.close()
        # 发生锁升级，集合锁为S锁
        self.check_is_lock_escalated(self.db, True)
        self.check_cl_lock_type(self.db, "S")

        # 事务中指定flags为SDB_FLG_QUERY_FOR_SHARE读取数据并校验
        cursor = dbcl.query(order_by={"a": 1}, flags=QUERY_FLG_FOR_SHARE)
        self.checkResult(cursor, docs)

        # 提交事务
        self.db.transaction_commit()

        # seqDB-25362:不开起事务使用SDB_FLG_QUERY_FOR_SHARE读数据
        cursor = dbcl.query(order_by={"a": 1}, flags=QUERY_FLG_FOR_SHARE)
        self.checkResult(cursor, docs)

    def tearDown(self):
        testlib.drop_cs(self.db, cs_neme, ignore_not_exist=True)

    def get_cl_lock_count(self, db, lock_type):
        cursor = db.get_snapshot(SDB_SNAP_TRANSACTIONS_CURRENT)
        lock_count = 0
        detail = testlib.get_all_records(cursor)
        for obj in detail[0]["GotLocks"]:
            if obj["CSID"] >= 0 and obj["CLID"] >= 0 and obj["ExtentID"] >= 0 and obj["Offset"] >= 0:
                if lock_type == obj["Mode"]:
                    lock_count += 1
        return lock_count

    def check_is_lock_escalated(self, db, is_lock_escalated):
        cursor = db.get_snapshot(SDB_SNAP_TRANSACTIONS_CURRENT)
        record = testlib.get_all_records(cursor)
        act_lock_escalated = record[0]["IsLockEscalated"]
        self.assertEqual(act_lock_escalated, is_lock_escalated)

    def check_cl_lock_type(self, db, lock_mode):
        cursor = db.get_snapshot(SDB_SNAP_TRANSACTIONS_CURRENT)
        detail = testlib.get_all_records(cursor)
        for obj in detail[0]["GotLocks"]:
            if obj["CSID"] >= 0 and obj["CLID"] >= 0 and obj["CLID"] != 65535 and obj["ExtentID"] == -1 and obj[
                "Offset"] == -1:
                self.assertEqual(obj["Mode"], lock_mode)

    def checkResult(self, cursor, expectRec):
        actRec = []
        while True:
            try:
                rec = cursor.next()
                del rec["_id"]
                actRec.append(rec)
            except SDBEndOfCursor:
                break
        self.assertListEqualUnordered(expectRec, actRec)

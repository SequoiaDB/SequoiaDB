# -*- coding: utf-8 -*-

# @testlink   seqDB-22899 [SEQUOIADBMAINSTREAM-6194]
# @interface  client.get_snapshot(); - SDB_SNAP_QUERIES,SDB_SNAP_LOCKWAITS
# @author     Zixian Yan 2020-10-19
import time
import threading
from lib import testlib
from lib import sdbconfig
from pysequoiadb import client
from pysequoiadb.error import (SDBBaseError, SDBEndOfCursor)

SDB_SNAP_CONFIGS = 13
SDB_SNAP_QUERIES = 18
SDB_SNAP_LOCKWAITS = 20


class Snapshot_22899(testlib.SdbTestBase):

    def setUp(self):
        # skip standlone mode
        # Cause some problems has not fix yet. Therefore Skip standlone mode for now.
        # Problems Order Number: SEQUOIADBMAINSTREAM-6354
        if testlib.is_standalone():
            self.skipTest("skip! This testcase does not support standlone")

        testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
        self.cs = self.db.create_collection_space(self.cs_name)
        self.cl = self.cs.create_collection(self.cl_name)

    def test_snapshot_22899(self):
        sel = {"mongroupmask": "", "transactionon": "", "transactiontimeout": "", "transisolation": "",
               "translockwait": "",
               "transautocommit": "", "transautorollback": "", "transuserbs": "", "transreplsize": "",
               "transrccount": ""}

        if testlib.is_standalone():
            original_conf = self.get_snapshot_result(SDB_SNAP_CONFIGS, selector=sel)[0]
        else:
            node = self.db.get_cata_replica_group().get_master()
            cata_host = node.get_hostname()
            cata_svc = node.get_servicename()
            self.node_name = cata_host + ":" + cata_svc
            original_conf = \
                self.get_snapshot_result(SDB_SNAP_CONFIGS, condition={"NodeName": self.node_name}, selector=sel)[0]

        try:
            configuration = {"mongroupmask": "all:detail", "transautocommit": "false", "transisolation": 2,
                             "transactiontimeout": 3, "translockwait": "true"}
            self.db.update_config(configs=configuration, options={})

            self.verify_snapshot_queries()
            self.verify_snapshot_lockwaits()
        finally:
            self.db.update_config(configs=original_conf, options={})

    def verify_snapshot_queries(self):
        result = self.get_snapshot_result(SDB_SNAP_QUERIES)
        if len(result) < 1:
            raise Exception("SDB_SNAP_QUERIES --- FAILED, return " + format(len(result)) + " information")

        filedType = list(result[0].keys())
        expectation = ["NodeID", "StartTimestamp", "EndTimestamp", "TID", "OpType", "Name", "QueryTimeSpent",
                       "ReturnNum", "TotalMsgSent", "LastOpInfo", "MsgSentTime", "RemoteNodeWaitTime", "ClientInfo",
                       "RelatedNode"]
        for i in expectation:
            if i not in filedType:
                raise Exception("SDB_SNAP_QUERIES field Type Error! \"" + format(i) + "\" is unexpected")

    def verify_snapshot_lockwaits(self):
        # Create an transaction lock wait
        db_2 = client(sdbconfig.sdb_config.host_name, sdbconfig.sdb_config.service)
        self.cl.insert({"TENET": "TENET"})

        self.db.transaction_begin()
        db_2.transaction_begin()

        self.cl.update({"$set": {"TENET": "tenet"}})
        try:
            db_2.get_collection_space(self.cs_name).get_collection(self.cl_name).query()

        except SDBBaseError as e:
            pass
        finally:
            self.db.transaction_commit()
            db_2.transaction_commit()
            time.sleep(3)
            db_2.close_all_cursors()

        result = self.get_snapshot_result(SDB_SNAP_LOCKWAITS, hint={"$Options": {"viewHistory": True}})
        if len(result) < 1:
            raise Exception("SDB_SNAP_LOCKWAITS --- FAILED, return " + format(len(result)) + " information")

        filedType = list(result[0].keys())
        expectation = ["NodeName", "WaiterTID", "RequiredMode", "CSID", "CLID", "ExtentID", "Offset", "StartTimestamp",
                       "TransLockWaitTime", "LatestOwner", "LatestOwnerMode", "NumOwner"]
        for i in expectation:
            if i not in filedType:
                raise Exception("SDB_SNAP_LOCKWAITS field Type Error! \"" + format(i) + "\" is unexpected")

    def get_snapshot_result(self, snap_type, **kwargs):
        result = []
        cursor = self.db.get_snapshot(snap_type, **kwargs)
        while True:
            try:
                rc = cursor.next()
                result.append(rc)
            except SDBEndOfCursor:
                break
        cursor.close()
        return result

    def tearDown(self):
        if self.should_clean_env():
            configuration = {"mongroupmask": 1, "transautocommit": 1, "transisolation": 1, "transactiontimeout": 1,
                             "translockwait": 1}
            self.db.delete_config(configs=configuration, options={})
            self.db.drop_collection_space(self.cs_name)

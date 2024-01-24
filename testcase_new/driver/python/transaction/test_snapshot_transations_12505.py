# @decription: test snapshot transaction, SEQUOIADBMAINSTREAM-2316
# @testlink:   seqDB-12505
# @interface:  get_snapshot(self,snap_type,kwargs)
#              reset_snapshot(self,condition)
# @author:     liuxiaoxuan 2018-02-22

import unittest
import datetime
from pysequoiadb.error import SDBBaseError
from lib import testlib

sdb_snap_transation = 9
sdb_snap_transation_current = 10
class TestSnapshotTransaction12505(testlib.SdbTestBase):
   def setUp(self):
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)

   def test_snapshot_transaction_12505(self):
   
      self.db.transaction_begin()
      self.insert_datas()
      
      # get snapshot 9
      expect_result = ["SessionID", "TransactionID"]
      cursor = self.db.get_snapshot(sdb_snap_transation)
      act_result = testlib.get_all_records(cursor)
      # check snapshot
      self.check_snapshot(expect_result, act_result)
      
      # get snapshot 10
      cursor = self.db.get_snapshot(sdb_snap_transation_current)
      act_result = testlib.get_all_records(cursor)
      # check snapshot
      self.check_snapshot(expect_result, act_result)
      
      self.db.transaction_commit()
		
   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)

   def insert_datas(self):
      doc = []
      for i in range(0, 10000):
         doc.append({"a": i})
      try:
         self.cl.bulk_insert(0, doc)
      except SDBBaseError as e:
         self.fail('insert fail: ' + str(e))
             
   def check_snapshot(self, expect_result, act_result):
      is_has_sessionid = False
      is_has_transactionid = False
      for x in act_result:
         if expect_result[0] in x:
            is_has_sessionid = True
         if expect_result[1] in x:
            is_has_transactionid = True
         self.assertTrue(is_has_sessionid)
         self.assertTrue(is_has_transactionid)

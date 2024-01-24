# @decription: test snapshot collectionSapces
# @testlink:   seqDB-12505
# @interface:  get_snapshot(self,snap_type,kwargs)
#              reset_snapshot(self,condition)
# @author:     liuxiaoxuan 2017-9-08

import unittest
import datetime
from pysequoiadb.error import SDBBaseError
import time
from lib import testlib

sdb_snap_collection_sapces = 5
class TestSnapshotCSCL12505(testlib.SdbTestBase):
   def setUp(self):
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)

   def test_snapshot_cscl_12505(self):
      condition = [{"Name": self.cs_name}, None]
      expect_result = {"Name": self.cs_name}

      # get snapshot with option
      act_result = self.get_snapshot_collectionspaces(condition[0])
      
      # check snapshot
      self.check_snapshot(expect_result, act_result)
      # get snapshot without option
      act_result = self.get_snapshot_collectionspaces(condition[1])
      # check snapshot
      self.check_snapshot(expect_result, act_result)
		
   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)

   def get_snapshot_collectionspaces(self, cond):
      rec = {}
      try:
         if cond == None:
            cursor = self.db.get_snapshot(sdb_snap_collection_sapces)
         else:
            cursor = self.db.get_snapshot(sdb_snap_collection_sapces, condition = cond)
         # get result
         rec = self.get_actual_result(cursor)
         # remark totalDataSize before reset snapshot
         self.oldDataSize = rec['TotalDataSize']
      except SDBBaseError as e:
         self.fail('get snapshot fail: ' + str(e))
      return rec
            
   def get_actual_result(self,cursor):
      rec = {}
      while True:
         try:
            rec = cursor.next()
            if self.cs_name == rec['Name']:
               rec = {'Name': rec['Name'], 'TotalDataSize': rec['TotalDataSize']}
               cursor.close()
               break
         except SDBEndOfCursor:
            cursor.close()
            break
      return rec

   def check_snapshot(self, expect_result, act_result):
      self.assertEqual(expect_result['Name'], act_result['Name'], str(expect_result) + " not equal " + str(act_result))
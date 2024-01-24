# @decription: test snapshot collections
# @testlink:   seqDB-12505
# @interface:  get_snapshot(self,snap_type,kwargs)
#              reset_snapshot(self,condition)
# @author:     liuxiaoxuan 2017-9-08

import unittest
import datetime
from pysequoiadb.error import SDBBaseError
from lib import testlib

sdb_snap_collections = 4
class TestSnapshotCollections12505(testlib.SdbTestBase):
   def setUp(self):
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)

   def test_snapshot_collections_12505(self):
   
      # get snapshot with option
      condition = {"Name": self.cs_name + "." + self.cl_name} 
      expect_result = [{"Name": self.cs_name + "." + self.cl_name}]
      cursor = self.db.get_snapshot(sdb_snap_collections, condition = condition)
      act_result = testlib.get_all_records(cursor)
      
      # check snapshot  
      self.check_snapshot(expect_result, act_result)
		
   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)
         
   def check_snapshot(self, expect_result, act_result):
      self.assertEqual(len(expect_result), len(act_result))
      for i in range(len(expect_result)):
         self.assertEqual(expect_result[i]['Name'], act_result[i]['Name'])
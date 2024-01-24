# @decription: test snapshot catalog
# @testlink:   seqDB-12505
# @interface:  get_snapshot(self,snap_type,kwargs)
#              reset_snapshot(self,condition)
# @author:     liuxiaoxuan 2017-9-08

import unittest
import datetime
from pysequoiadb.error import (SDBTypeError, SDBBaseError, SDBEndOfCursor, SDBError)
import time
from lib import testlib

sdb_snap_catalog = 8  
class TestSnapshotCatalog12505(testlib.SdbTestBase):
   def setUp(self):
      # skip standalone
      if testlib.is_standalone():
         self.skipTest('run mode is standalone')
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)  
      self.cs = self.db.create_collection_space(self.cs_name)
  
   def test_snapshot_catalog_12505(self):
      # create cl
      groups = testlib.get_data_groups()
      group_name = groups[0]['GroupName']
      self.cs.create_collection(self.cl_name, {'Group': group_name})
      
      # get snapshot with option
      condition = {"Name": self.cs_name + "." + self.cl_name} 
      expect_result = [{"Name": self.cs_name + "." + self.cl_name, "GroupName": group_name}]
      cursor = self.db.get_snapshot(sdb_snap_catalog, condition = condition)
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
         self.assertEqual(expect_result[i]['GroupName'], act_result[i]['CataInfo'][0]['GroupName'])
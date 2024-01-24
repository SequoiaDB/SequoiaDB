# @testlink:   seqDB-12505
# @interface:  get_snapshot(self,snap_type,kwargs)
# @author:     yinzhen 2019-10-24

import unittest
from pysequoiadb.client import (SDB_SNAP_CATALOG, SDB_SNAP_SEQUENCES)
from lib import testlib

class TestSnapshotSequences12505(testlib.SdbTestBase):
   def setUp(self):
      if testlib.is_standalone():
         self.skipTest('run mode is standalone')
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)

   def test_snapshot_sequences_12505(self):
  
      self.cl.create_autoincrement({"Field":"a", "Increment":1})

      # get snapshot with option
      cursor = self.db.get_snapshot(SDB_SNAP_CATALOG, condition={"Name":self.cs_name + "." + self.cl_name})
      ret_results = testlib.get_all_records(cursor)
      ret_result = ret_results[0]
      seq_name = ret_result["AutoIncrement"][0]["SequenceName"]

      expect_result = {"CurrentValue":1, "StartValue":1, "Increment":1, "CacheSize":1000, "AcquireSize":1000, "Cycled":False, "Initial":True}
      cursor = self.db.get_snapshot(SDB_SNAP_SEQUENCES, condition={"Name":seq_name})
      act_results = testlib.get_all_records(cursor)
      act_result = act_results[0]

      # check snapshot  
      self.check_snapshot(expect_result, act_result)

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)

   def check_snapshot(self, expect_result, act_result):
      for field in expect_result:
         self.assertEqual(expect_result[field], act_result[field])

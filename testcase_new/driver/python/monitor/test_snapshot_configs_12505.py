# @decription: test snapshot configs
# @testlink:   seqDB-12505
# @interface:  get_snapshot(self,snap_type,kwargs)
# @author:     yinzhen 2019-10-24

import unittest
from pysequoiadb.client import (SDB_SNAP_CONFIGS)
from lib import testlib

class TestSnapshotConfigs12505(testlib.SdbTestBase):
   def setUp(self):
      pass

   def test_snapshot_configs_12505(self):

      # get snapshot with option
      expect_result = {"transactiontimeout":60, "transactionon":"TRUE", "translockwait":"FALSE"}
      cursor = self.db.get_snapshot(SDB_SNAP_CONFIGS)
      act_results = testlib.get_all_records(cursor)
      act_result = act_results[0]

      # check snapshot 
      self.check_snapshot(expect_result, act_result)

   def tearDown(self):
      pass

   def check_snapshot(self, expect_result, act_result):
      for field in expect_result:
         self.assertEqual(expect_result[field], act_result[field])

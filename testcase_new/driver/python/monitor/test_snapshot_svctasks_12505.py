# @decription: test snapshot configs
# @testlink:   seqDB-12505
# @interface:  get_snapshot(self,snap_type,kwargs)
# @author:     yinzhen 2019-10-24

import unittest
from pysequoiadb.client import (SDB_SNAP_SVCTASKS)
from lib import testlib

class TestSnapshotSvctasks12505(testlib.SdbTestBase):
   def setUp(self):
      pass

   def test_snapshot_svctasks_12505(self):

      # get snapshot with option
      expect_result = {"TaskName":"Default", "TaskID":0}
      cursor = self.db.get_snapshot(SDB_SNAP_SVCTASKS, condition={"TaskName":"Default"})
      act_results = testlib.get_all_records(cursor)
      act_result = act_results[0]

      # check snapshot 
      self.check_snapshot(expect_result, act_result)

   def tearDown(self):
      pass

   def check_snapshot(self, expect_result, act_result):
      for field in expect_result:
         self.assertEqual(expect_result[field], act_result[field])

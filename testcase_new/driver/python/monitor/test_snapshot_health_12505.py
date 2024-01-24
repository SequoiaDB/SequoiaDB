# @decription: test snapshot health
# @testlink:   seqDB-12505
# @interface:  get_snapshot(self,snap_type,kwargs)
# @author:     liuxiaoxuan 2018-02-22

import unittest
import datetime
from pysequoiadb.error import SDBBaseError
from lib import testlib

sdb_snap_health = 12
class TestSnapshotHealth12505(testlib.SdbTestBase):
   def setUp(self):
      pass

   def test_snapshot_health_12505(self):
      expect_result = ["DataStatus", "SyncControl"]
      # snapshot 7
      cursor = self.db.get_snapshot(sdb_snap_health)
      act_result = testlib.get_all_records(cursor)
      # check snapshot
      self.check_snapshot(expect_result, act_result)
		
   def tearDown(self):
      pass

   def check_snapshot(self, expect_result, act_result):
      is_has_datastatus = False
      is_has_synccontrol = False
      for x in act_result:
         if expect_result[0] in x:
            is_has_datastatus = True
         if expect_result[1] in x:
            is_has_synccontrol = True
         self.assertTrue(is_has_datastatus)
         self.assertTrue(is_has_synccontrol)
         
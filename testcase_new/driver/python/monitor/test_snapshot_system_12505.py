# @decription: test snapshot system
# @testlink:   seqDB-12505
# @interface:  get_snapshot(self,snap_type,kwargs)
# @author:     liuxiaoxuan 2018-02-22

import unittest
import datetime
from pysequoiadb.error import SDBBaseError
from lib import testlib

sdb_snap_system = 7
class TestSnapshotSystem12505(testlib.SdbTestBase):
   def setUp(self):
      pass

   def test_snapshot_system_12505(self):
      expect_result = ["CPU", "Memory"]
      # snapshot 7
      cursor = self.db.get_snapshot(sdb_snap_system)
      act_result = testlib.get_all_records(cursor)
      # check snapshot
      self.check_snapshot(expect_result, act_result)
		
   def tearDown(self):
      pass

   def check_snapshot(self, expect_result, act_result):
      is_has_cpu = False
      is_has_memory = False
      for x in act_result:
         if expect_result[0] in x:
            is_has_cpu = True
         if expect_result[1] in x:
            is_has_memory = True
         self.assertTrue(is_has_cpu)
         self.assertTrue(is_has_memory)
         
# @decription: test snapshot contexts
# @testlink:   seqDB-12505
# @interface:  get_snapshot(self,snap_type,kwargs)
# @author:     liuxiaoxuan 2018-02-22

import unittest
import datetime
from pysequoiadb.error import SDBBaseError
from lib import testlib

sdb_snap_contexts = 0
sdb_snap_contexts_current = 1
class TestSnapshotContexts12505(testlib.SdbTestBase):
   def setUp(self):
      pass

   def test_snapshot_contexts_12505(self):
      expect_result = ["SessionID", "Contexts"]
      # snapshot 0
      cursor = self.db.get_snapshot(sdb_snap_contexts)
      act_result = testlib.get_all_records(cursor)
      # check snapshot
      self.check_snapshot(expect_result, act_result)
      
      # snapshot 1
      cursor = self.db.get_snapshot(sdb_snap_contexts_current)
      act_result = testlib.get_all_records(cursor)
      # check snapshot
      self.check_snapshot(expect_result, act_result)
		
   def tearDown(self):
      pass

   def check_snapshot(self, expect_result, act_result):
      is_has_sessionid = False
      is_has_contexts = False
      for x in act_result:
         if expect_result[0] in x:
            is_has_sessionid = True
         if expect_result[1] in x:
            is_has_contexts = True
         self.assertTrue(is_has_sessionid)
         self.assertTrue(is_has_contexts)
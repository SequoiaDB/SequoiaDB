# @decription: test snapshot sessions
# @testlink:   seqDB-12505
# @interface:  get_snapshot(self,snap_type,kwargs)
# @author:     liuxiaoxuan 2018-02-22

import unittest
import datetime
from pysequoiadb.error import SDBBaseError
from lib import testlib

sdb_snap_sessions = 2
sdb_snap_sessions_current = 3
class TestSnapshotSessions12505(testlib.SdbTestBase):
   def setUp(self):
      pass

   def test_snapshot_sessions_12505(self):
      expect_result = ["SessionID", "TID", "Type"]
      # snapshot 2
      cursor = self.db.get_snapshot(sdb_snap_sessions)
      act_result = testlib.get_all_records(cursor)
      # check snapshot
      self.check_snapshot(expect_result, act_result)
      
      # snapshot 3
      cursor = self.db.get_snapshot(sdb_snap_sessions_current)
      act_result = testlib.get_all_records(cursor)
      # check snapshot
      self.check_snapshot(expect_result, act_result)
		
   def tearDown(self):
      pass

   def check_snapshot(self, expect_result, act_result):
      is_has_sessionid = False
      is_has_tid = False
      is_has_type = False
      for x in act_result:
         if expect_result[0] in x:
            is_has_sessionid = True
         if expect_result[1] in x:
            is_has_tid = True
         if expect_result[2] in x:
            is_has_type = True
         self.assertTrue(is_has_sessionid)
         self.assertTrue(is_has_tid)
         self.assertTrue(is_has_type)
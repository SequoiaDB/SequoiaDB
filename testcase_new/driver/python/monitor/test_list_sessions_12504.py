# @decription: test list sessions
# @testlink:   seqDB-12504
# @interface:  get_list(self,list_type,kwargs)
# @author:     liuxiaoxuan 2017-02-22

import unittest
import datetime
from pysequoiadb.error import SDBBaseError
from lib import testlib

sdb_list_sessions = 2
sdb_list_sessions_current = 3
class TestListSessions12504(testlib.SdbTestBase):
   def setUp(self):
      pass

   def test_list_sessions_12504(self):
      expect_result = ["SessionID", "TID", "Type"]
      # list 2
      cursor = self.db.get_list(sdb_list_sessions)
      act_result = testlib.get_all_records(cursor)
      # check list
      self.check_list(expect_result, act_result)
      
      # list 3
      cursor = self.db.get_list(sdb_list_sessions_current)
      act_result = testlib.get_all_records(cursor)
      # check list
      self.check_list(expect_result, act_result)
		
   def tearDown(self):
      pass

   def check_list(self, expect_result, act_result):
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
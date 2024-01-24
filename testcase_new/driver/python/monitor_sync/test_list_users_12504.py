# @decription: test list users
# @testlink:   seqDB-12504
# @interface:  get_list(self,list_type,kwargs)
# @author:     yinzhen 2019-10-24

import unittest
from lib import testlib
from pysequoiadb.client import (SDB_LIST_USERS)

class TestListUsers12504(testlib.SdbTestBase):
   def setUp(self):
      self.db.create_user("test_user_12504", "test_user_12504")

   def test_list_users_12504(self):

      # get list with option
      expect_result = {"User":"test_user_12504"}
      cursor = self.db.get_list(SDB_LIST_USERS, condition={"User":"test_user_12504"})
      act_results = testlib.get_all_records(cursor)
      act_result = act_results[0]
      
      # check list
      self.check_list(expect_result, act_result)

   def tearDown(self):
      self.db.remove_user("test_user_12504", "test_user_12504")
      
   def check_list(self, expect_result, act_result):
      for field in expect_result:
         self.assertEqual(expect_result[field], act_result[field])

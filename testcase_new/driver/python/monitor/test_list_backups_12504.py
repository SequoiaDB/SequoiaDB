# @decription: test list backups
# @testlink:   seqDB-12504
# @interface:  get_list(self,list_type,kwargs)
# @author:     yinzhen 2019-10-24

import unittest
from lib import testlib
from pysequoiadb.client import (SDB_LIST_BACKUPS)

class TestListBackups12504(testlib.SdbTestBase):
   def setUp(self):
      pass

   def test_list_backups_12504(self):

      self.db.backup({"Name":"test_12504_backup"})

      # get list with option
      expect_result = {"Name":"test_12504_backup"}
      cursor = self.db.get_list(SDB_LIST_BACKUPS, condition={"Name":"test_12504_backup"})
      act_results = testlib.get_all_records(cursor)
      act_result = act_results[0]
      
      # check list
      self.check_list(expect_result, act_result)

   def tearDown(self):
      self.db.remove_backup({"Name":"test_12504_backup"})
      
   def check_list(self, expect_result, act_result):
      for field in expect_result:
         self.assertEqual(expect_result[field], act_result[field])

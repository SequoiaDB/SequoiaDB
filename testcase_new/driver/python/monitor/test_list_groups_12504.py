# @decription: test list groups
# @testlink:   seqDB-12504
# @interface:  get_list(self,list_type,kwargs)
# @author:     liuxiaoxuan 2017-02-22

import unittest
import datetime
from pysequoiadb.error import SDBBaseError
from lib import testlib

sdb_list_groups = 7
class TestListGroups12504(testlib.SdbTestBase):
   def setUp(self):
      # skip standalone
      if testlib.is_standalone():
         self.skipTest('run mode is standalone')
      
   def test_list_groups_12504(self):
   
      expect_result = ["Group", "GroupName"]
      # list 7
      cursor = self.db.get_list(sdb_list_groups)
      act_result = testlib.get_all_records(cursor)
      # check list
      self.check_list(expect_result, act_result)
      		
   def tearDown(self):
      pass
         
   def check_list(self, expect_result, act_result):
      is_group = False
      is_groupname = False
      for x in act_result:
         if expect_result[0] in x:
            is_group = True
         if expect_result[1] in x:
            is_groupname = True
         self.assertTrue(is_group)
         self.assertTrue(is_groupname)

# @decription: test list svctasks
# @testlink:   seqDB-12504
# @interface:  get_list(self,list_type,kwargs)
# @author:     yinzhen 2019-10-24

import unittest
from pysequoiadb.error import (SDBTypeError, SDBBaseError, SDBEndOfCursor, SDBError)
from lib import testlib
from pysequoiadb.client import (SDB_LIST_SVCTASKS)

class TestListSvctasks12504(testlib.SdbTestBase):
   def setUp(self):
      pass

   def test_list_svctasks_12504(self):

      # get list with option
      expect_result = {"TaskName":"Default", "TaskID":0}
      cursor = self.db.get_list(SDB_LIST_SVCTASKS, condition={"TaskName":"Default"})
      act_results = testlib.get_all_records(cursor)
      act_result = act_results[0]
      
      # check list
      self.check_list(expect_result, act_result)

   def tearDown(self):
      pass 
     
   def check_list(self, expect_result, act_result):
      for field in expect_result:
         self.assertEqual(expect_result[field], act_result[field])

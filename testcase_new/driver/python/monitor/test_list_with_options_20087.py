# @decription: test list with skip/limit/hint
# @testlink:   seqDB-20087
# @interface:  get_list(self,list_type,kwargs)
# @author:     yinzhen 2019-10-25

import unittest
from lib import testlib
from pysequoiadb.client import (SDB_LIST_COLLECTIONS)
from bson.py3compat import (long_type)

class TestListCollections20087(testlib.SdbTestBase):
   def setUp(self):
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)
      for i in range(10):
         self.cs.create_collection("test_20087_" + str(i))

   def test_list_collections_20087(self):

      # get list with option
      expect_result=3
      cursor = self.db.get_list(SDB_LIST_COLLECTIONS, num_to_return=long_type(3), num_to_skip=long_type(2))
      act_result = testlib.get_all_records(cursor)
      
      # check list
      self.check_list(expect_result, act_result)

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)

   def check_list(self, expect_result, act_result):
      self.assertEqual(expect_result, len(act_result))

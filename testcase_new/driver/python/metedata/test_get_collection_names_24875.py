# -*- coding: utf-8 -*-
# @decription: seqDB-24875:驱动支持cs.getCollectionNames
# @author:     liuli
# @createTime: 2021.12.22

import unittest
from lib import testlib
from pysequoiadb.error import SDBBaseError

cs_neme = "cs_24875"
class TestGetCollectionNames24875(testlib.SdbTestBase):
   def setUp(self):
      testlib.drop_cs(self.db, cs_neme, ignore_not_exist=True)
      
   def test_dropCS_24875(self):
      cl_name = "cl_24875_"

      exp_names = []
      dbcs = self.db.create_collection_space(cs_neme)
      cl_names = dbcs.get_collection_names()
      if cl_names != exp_names:
         raise Exception("expect cl name is ",exp_names,", actual cl name is ",cl_names)

      dbcs.create_collection(cl_name + "0")
      exp_names.append(cs_neme + "." + cl_name + "0")
      cl_names = dbcs.get_collection_names()
      if cl_names != exp_names:
         raise Exception("expect cl name is ", exp_names, ", actual cl name is ", cl_names)

      for i in range(1, 50):
         dbcs.create_collection(cl_name + str(i))
         exp_names.append(cs_neme + "." + cl_name + str(i))

      cl_names = dbcs.get_collection_names()
      cl_names.sort()
      exp_names.sort()
      if cl_names != exp_names:
         raise Exception("expect cl name is ", exp_names, ", actual cl name is ", cl_names)

   def tearDown(self):
      testlib.drop_cs(self.db, cs_neme, ignore_not_exist=True)
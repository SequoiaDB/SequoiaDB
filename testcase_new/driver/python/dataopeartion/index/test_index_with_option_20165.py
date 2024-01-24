# @decription: create common index
# @testlink:   seqDB-20165
# @interface:  create_index_with_option(self,index_def,idx_name,option=None)
# @author:     yinzhen 2019-11-4

from lib import testlib
from pysequoiadb.error import (SDBBaseError)

class TestIndexWithOption20165(testlib.SdbTestBase):
   def setUp(self):
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)

   def test_index_with_option_20165(self):

      # test record is not null
      self.cl.insert({"a":1})
      self.cl.create_index_with_option({"a":1}, "idx", {"NotNull":True})
      self.assertEqual(1, self.cl.get_count())
      self.cl.drop_index("idx")
      self.cl.truncate()
  
      # test record is null
      self.cl.insert({"a":None})
      try:
         self.cl.create_index_with_option({"a":1}, "idx", {"NotNull":True})
         self.fail("need throw fail")
      except SDBBaseError as e:
         self.assertEqual(-339, e.code)
      self.cl.create_index_with_option({"a":1}, "idx", {"NotNull":False})
      self.assertEqual(1, self.cl.get_count())
      self.cl.drop_index("idx")
      self.cl.truncate()

      # test record is not exist
      self.cl.insert({"b":1})
      try:
         self.cl.create_index_with_option({"a":1}, "idx", {"NotNull":True})
         self.fail("need throw fail")
      except SDBBaseError as e:
         self.assertEqual(-339, e.code)
      self.cl.create_index_with_option({"a":1}, "idx", {"NotNull":False})
      self.assertEqual(1, self.cl.get_count())
      self.cl.drop_index("idx")
      self.cl.truncate()

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)


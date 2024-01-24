# @decription: create exist index, drop non exist index
# @testlink:   seqDB-13681
# @author:     liuxiaoxuan 2017-12-07

from lib import testlib
from pysequoiadb.error import (SDBBaseError, SDBError)

class IndexException13681(testlib.SdbTestBase):
   def setUp(self):
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)

   def test_index_13681(self):
      # create non exist index
      index_name = "a"
      key = {'a' : 1}
      self.cl.create_index(key, index_name)

      # check create index success
      self.check_create_index_success(index_name)

      # check create exist index
      self.check_error_create_index(index_name, key)

      # drop exist index
      self.cl.drop_index(index_name)

      # check drop index success
      self.check_drop_index_success(index_name)

      # check drop non exist index
      self.check_error_drop_index(index_name)

   def check_create_index_success(self, index_name):
      try:
         rec = self.cl.get_indexes(index_name).next()
         act_index = rec['IndexDef']['name']
         self.assertEqual(index_name, act_index)
      except SDBBaseError as e:
         self.fail("check create index fail: " + e.detail)

   def check_drop_index_success(self, index_name):
      try:
         cursor = self.cl.get_indexes(index_name)
         act_index = testlib.get_all_records_noid(cursor)
         self.assertEqual(0, len(act_index))
      except SDBBaseError as e:
         self.fail("check create index fail: " + e.detail)

   def check_error_create_index(self, index_name, key):
      try:
         self.cl.create_index(key, index_name)
         self.fail("NEED SDB ERROR")
      except SDBError as e:
         self.assertEqual(e.code, -247)

   def check_error_drop_index(self, index_name):
      try:
         self.cl.drop_index(index_name)
         self.fail("NEED SDB ERROR")
      except SDBError as e:
         self.assertEqual(e.code, -47)

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)

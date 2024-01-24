# @decription: find records with flag:prepare more
# @testlink:   seqDB-13657
# @interface:  query(self,kwargs)
# @author:     liuxiaoxuan 2017-11-30

from bson.py3compat import (long_type)
from pysequoiadb import collection
from lib import testlib
from pysequoiadb.error import (SDBBaseError, SDBEndOfCursor)

class TestFind13657(testlib.SdbTestBase):
   def setUp(self):
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)
      self.insert_datas()

   def test_find_13657(self):
      # query with flags
      cond = {"a": {'$gt': 98}}
      selection = {"a": {"$include": 1}}
      flag = collection.QUERY_PREPARE_MORE
      expResult = [{"a": 99}]
      
      self.check_query_result(expResult,
                              condition = cond, 
                              selector = selection,
                              order_by = {"a": 1},
                              flags = flag)

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)

   def insert_datas(self):
      doc = []
      for i in range(0, 100):
         doc.append({"a": i})
      try:
         flags = 0
         self.cl.bulk_insert(flags, doc)
      except SDBBaseError as e:
         self.fail('insert fail: ' + str(e))
         
   def check_query_result(self, expectResult, **kwargs):
      try:
         cursor = self.cl.query(**kwargs)

         actResult = testlib.get_all_records_noid(cursor)
         self.assertListEqualUnordered(expectResult, actResult)
      except SDBBaseError as e:
         self.fail('check query fail: ' + str(e))
    
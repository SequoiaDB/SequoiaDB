# @decription: find records with option FLAGS
# @testlink:   seqDB-12394
# @interface:  query(self,kwargs)
#              query_one(self,kwargs)
# @author:     liuxiaoxuan 2017-8-29

from lib import testlib
from pysequoiadb.error import (SDBBaseError, SDBEndOfCursor, SDBError)

insert_nums = 100
class TestFind12394(testlib.SdbTestBase):
   def setUp(self):
      testlib.drop_cs(self.db,self.cs_name,ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)
      self.insert_datas()

   def test_find_12394(self):
      # query all
      expectAllRec = []
      for i in range(0, insert_nums):
         expectAllRec.append({"_id": i, "a": "test" + str(i)})

      flag_0 = 0
      self.query_all(expectAllRec, flag_0)
      flag_1 = 1
      self.query_all(expectAllRec, flag_1)

      # query one
      condition = {"_id": {"$gt": 5}}
      expectOneRec = {"_id": 6, "a": "test6"}
      flag_10 = 10
      self.query_one(expectOneRec, condition, flag_10)

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)
		
   def insert_datas(self):
      flag = 0
      doc = []
      for i in range(0, insert_nums):
         doc.append({"_id": i, "a": "test" + str(i)})
      try:
         self.cl.bulk_insert(flag, doc)
      except SDBError as e:
         self.fail('insert fail: ' + str(e))

   def query_all(self, expectRec, flag):
      try:
         sort = {"_id": 1}
         cursor = self.cl.query(order_by = sort, flags = flag)
         actRec = []
         while True:
            try:
               rec = cursor.next()
               actRec.append(rec)
            except SDBEndOfCursor:
               break
         self.assertListEqualUnordered(expectRec, actRec)
      except SDBBaseError as e:
         self.fail("query all error: " + str(e))

   def query_one(self, expectRec, cond, flag):
      try:
         sort = {"_id": 1}
         rec = self.cl.query_one(order_by = sort, condition = cond, flags = flag)
         self.assertEqual(rec, expectRec)
      except SDBBaseError as e:
         self.fail("query one error: " + str(e))

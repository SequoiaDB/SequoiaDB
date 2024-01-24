# @decription: data  opeartion
# @testlink:   seqDB-12465
# @author:     LaoJingTang 2017-8-30

from lib import testlib
from pysequoiadb.collection import (UPDATE_FLG_RETURNNUM)

NUM = 10


class Data12465Sdb(testlib.SdbTestBase):
   def setUp(self):
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)

   def upsert_test(self, record_to_insert, cl_list__expect, upsert_rule, **kwargs):
      self.cl.bulk_insert(0, record_to_insert)

      self.cl.upsert(upsert_rule, **kwargs)
      list1 = testlib.get_all_records_noid(self.cl.query())
      self.assertListEqualUnordered(cl_list__expect, list1)
      self.cl.delete()

   def test12465(self):
      record_to_insert = [{"a": 1} for i in range(NUM)]

      upsert_rule = {"$inc": {"a": 1}}

      # condition+upsert
      condition = {"a": {"$et": 1}}
      expect = [{"a": 2} for i in range(NUM)]
      self.upsert_test(record_to_insert, expect, upsert_rule, condition=condition)

       # test upsert with UPDATE_FLG_RETURNNUM
      upsert_rule = {"$set": {"a": 2*NUM}}
      condition = {"a": {"$gt": 2*NUM}}
      ret_value = self.cl.upsert(upsert_rule, condition=condition, flags=UPDATE_FLG_RETURNNUM)
      self.assertEqual({"ModifiedNum": 0, "UpdatedNum": 0, "InsertedNum": 1}, ret_value)
      self.assertEqual(self.cl.get_count({"a":  2*NUM}), 1)

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)

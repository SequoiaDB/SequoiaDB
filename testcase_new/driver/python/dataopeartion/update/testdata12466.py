# @decription: data  opeartion
# @testlink:   seqDB-12466
# @author:     LaoJingTang 2017-8-30

from lib import testlib

NUM = 10


class Data12466Sdb(testlib.SdbTestBase):
   def setUp(self):
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)

   def upsert_test(self, record_to_insert, cl_list__expect, upsert_rule, **kwargs):
      self.cl.bulk_insert(0, record_to_insert)
      if "return_new" not in kwargs:
         kwargs["return_new"] = True

      self.cl.upsert(upsert_rule, **kwargs)
      list1 = testlib.get_all_records_noid(self.cl.query())
      self.assertListEqualUnordered(cl_list__expect, list1)
      self.cl.delete()

   def test12466(self):
      record_to_insert = [{"a": i, "b": i} for i in range(NUM)]
      expect = [{"a": i, "b": i} for i in range(NUM)]
      expect.append({"a": 100})

      upsert_rule = {"$inc": {"a": 1}}

      # condition+upsert
      condition = {"a": {"$et": 99}}
      self.upsert_test(record_to_insert, expect, upsert_rule, condition=condition)

      # hint+upsert
      hint = {"": "index"}
      expect = [{"a": i + 1, "b": i} for i in range(10)]
      self.upsert_test(record_to_insert, expect, upsert_rule, hint=hint)

      # flags+upsert
      QUERY_FLG_FORCE_HINT = 128
      self.upsert_test(record_to_insert, expect, upsert_rule, flagss=QUERY_FLG_FORCE_HINT, hint=hint)

      # setOnInsert+upsert
      setOnInsert = {"a": "aaa"}
      expect = [{"a": i, "b": i} for i in range(10)]
      expect.append({"a": "aaa"})
      self.upsert_test(record_to_insert, expect, upsert_rule, condition=condition, setOnInsert=setOnInsert)

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)

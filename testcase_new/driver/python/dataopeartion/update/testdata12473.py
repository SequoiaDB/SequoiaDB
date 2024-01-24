# @decription: data  opeartion
# @testlink:   seqDB-12473
# @author:     LaoJingTang 2017-8-30

from lib import testlib


class Data12473Sdb(testlib.SdbTestBase):
   def setUp(self):
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)

   def query_update_test(self, cl_list__expect, return_list_expect, update_rule, **kwargs):
      records = [{"a": i, "b": i} for i in range(10)]
      self.cl.bulk_insert(0, records)

      if "return_new" not in kwargs:
         kwargs["return_new"] = True

      cur = self.cl.query_and_update(update_rule, **kwargs)
      list2 = testlib.get_all_records_noid(cur)
      list1 = testlib.get_all_records_noid(self.cl.query())
      self.assertListEqualUnordered(cl_list__expect, list1)
      self.assertListEqualUnordered(return_list_expect, list2)
      self.cl.delete()

   def test12473(self):
      original_list = [{"a": i, "b": i} for i in range(10)]

      update_rule = {"$inc": {"a": 1}}

      # condition+update
      condition = {"a": {"$et": 1}}
      return_list_expect = [{"a": 2, "b": 1}]
      expect = list(original_list)
      expect[1] = {"a": 2, "b": 1}
      self.query_update_test(expect, return_list_expect, update_rule, condition=condition)

      # selector+update
      expect_list = [{"a": i + 1, "b": i} for i in range(10)]
      expect_return_list = [{"a": i + 1} for i in range(10)]
      selector = {"b": {"$include": 0}}
      self.query_update_test(expect_list, expect_return_list, update_rule, selector=selector)

      # order_by+update
      order_by = {"_id": -1}
      self.query_update_test(expect_list, expect_list, update_rule, order_by=order_by)

      # num_to_skip+update
      num_to_skip = 5
      l = list()
      for i in range(10):
         if i >= 5:
            l.append({"a": i + 1, "b": i})
         else:
            l.append({"a": i, "b": i})
      self.query_update_test(l, expect_list[num_to_skip:], update_rule, num_to_skip=num_to_skip)

      # num_to_return+update
      num_to_return = 5
      l = list()
      for i in range(10):
         if i < 5:
            l.append({"a": i + 1, "b": i})
         else:
            l.append({"a": i, "b": i})
      self.query_update_test(l, expect_list[:num_to_return], update_rule, num_to_return=num_to_return)

      # hint+update
      hint = {"": "index"}
      self.query_update_test(expect_list, expect_list, update_rule, hint=hint)

      # selector+update
      selector = {"b": {"$include": 0}}
      self.query_update_test(expect_list, [{"a": i + 1} for i in range(10)], update_rule, selector=selector)

      # return_new+update
      self.query_update_test(expect_list, original_list, update_rule, return_new=False)

      # flags+update
      QUERY_FLG_FORCE_HINT = 128
      self.query_update_test(expect_list, expect_list, update_rule, flagss=QUERY_FLG_FORCE_HINT, hint=hint)

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)

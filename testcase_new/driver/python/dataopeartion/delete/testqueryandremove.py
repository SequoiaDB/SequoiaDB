# -- coding: utf-8 --
"""
 @decription:
 @testlink:   测试用例 seqDB-12472 :: 版本: 1 :: 查询并删除记录
 @author:     laojingtang
"""
from lib import testlib
from pysequoiadb import collection

default_list = [{"a": i} for i in range(10)]


class SdbTestQueryAndUpdate(testlib.SdbTestBase):
   def setUp(self):
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)

   # 子测试
   def __sub_test(self, expect, expect_return, insert=None, **kwargs):
      if "flags" not in kwargs:
         kwargs.update({"falgs": collection.QUERY_FLG_WITH_RETURNDATA})

      cl = self.cl
      if insert == None:
         insert = default_list
      cl.bulk_insert(0, insert)

      cur = cl.query_and_remove(**kwargs)
      rl = testlib.get_all_records_noid(cur)
      l = testlib.get_all_records_noid(cl.query())
      self.assertListEqualUnordered(expect, l)
      self.assertListEqualUnordered(expect_return, rl)
      cl.truncate()

   def test_query_and_update(self):
      # remove all
      e = []
      re = default_list
      self.__sub_test([], re)

      # remove + condition
      condition = {"a": 0}
      e = [{"a": i} for i in range(1, 10)]
      re = [{"a": 0}]
      self.__sub_test(e, re, condition=condition)

      condition = {"$and": [{"a": 0}, {"b": 0}]}
      list_insert = [{"a": i, "b": i} for i in range(10)]
      e = [{"a": i, "b": i} for i in range(1, 10)]
      re = [{"a": 0, "b": 0}]
      self.__sub_test(e, re, list_insert, condition=condition)

      list_insert = [{"a": {"a": 1}}, {"a": {"b": 1}}]
      condition = {"a": {"$elemMatch": {"a": 1}}}
      e = [{"a": {"b": 1}}]
      re = [{"a": {"a": 1}}]
      self.__sub_test(e, re, list_insert, condition=condition)

      # remove + selector
      selector = {"b": {"$include": 0}}
      i_list = [{"a": i, "b": i} for i in range(10)]
      e = []
      re = [{"a": i} for i in range(10)]
      self.__sub_test(e, re, insert=i_list, selector=selector)

      # remove + order_by
      order_by = {"_id": -1}
      i_list = default_list
      e = []
      re = default_list
      self.__sub_test(e, re, insert=i_list, order_by=order_by)

      # remove + num_to_skip
      num_to_skip = 5
      i_list = default_list
      e = i_list[0:5]
      re = i_list[5:10]
      self.__sub_test(e, re, insert=i_list, num_to_skip=num_to_skip)

      # remove + hint
      hint = {"": "index"}
      i_list = default_list
      e = []
      re = i_list
      self.__sub_test(e, re, insert=i_list, hint=hint)

      # remove + num_to_return
      num_to_return = 5
      i_list = default_list
      e = i_list[5:10]
      re = i_list[0:5]
      self.__sub_test(e, re, insert=i_list, num_to_return=num_to_return)

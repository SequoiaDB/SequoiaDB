# -- coding: utf-8 --
"""
 @decription:
 @testlink:   seqDB-12469 :: 版本: 1 :: 指定条件删除记录
              seqDB-12472 :: 版本: 1 :: 查询并删除记录
 @author:     laojingtang
"""
from lib import testlib
from pysequoiadb.collection import (INSERT_FLG_RETURN_OID,DELETE_FLG_RETURNNUM)


class SdbTestDelete12472(testlib.SdbTestBase):
   def setUp(self):
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)

   def __delete_test(self, insert_list, expect, **kwargs):
      cl = self.cl
      cl.bulk_insert(0, insert_list)
      cl.delete(**kwargs)
      r = testlib.get_all_records_noid(cl.query())
      self.assertListEqualUnordered(expect, r)
      cl.truncate();

   def test_delete(self):
      list_insert = [{"a": i} for i in range(10)]
      # test delete all
      self.__delete_test(list_insert, [])
      # test delete with condition
      condition = {"a": 0}
      self.__delete_test(list_insert, [{"a": i} for i in range(1, 10)], condition=condition)

      condition = {"$and": [{"a": 0}, {"b": 0}]}
      list_insert = [{"a": i, "b": i} for i in range(10)]
      e = [{"a": i, "b": i} for i in range(1, 10)]
      self.__delete_test(list_insert, e, condition=condition)
      list_insert = [{"a": {"a": 1}}, {"a": {"b": 1}}]
      condition = {"a": {"$elemMatch": {"a": 1}}}
      self.__delete_test(list_insert, [{"a": {"b": 1}}], condition=condition)

      # test delete with hint
      list_insert = [{"a": i} for i in range(10)]
      hint = {"": "index"}
      self.__delete_test(list_insert, [], hint=hint)

      # test delete with DELETE_FLG_RETURNNUM
      list_insert = [{"a": i} for i in range(10)]
      self.cl.bulk_insert(INSERT_FLG_RETURN_OID, records=list_insert)
      ret_value = self.cl.delete(condition={}, flags=DELETE_FLG_RETURNNUM)
      self.assertEqual({"DeletedNum": 10}, ret_value)
      self.assertEqual(self.cl.get_count(), 0)


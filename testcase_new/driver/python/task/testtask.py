# -- coding: utf-8 --
"""
 @decription:
 @testlink:   测试用例 seqDB-12495 :: 版本: 1 :: 列出/取消/等待任务
 @author:     laojingtang
"""
from pysequoiadb import SDBError
from pysequoiadb.errcode import SDB_CAT_TASK_NOTFOUND, SDB_TASK_ALREADY_FINISHED, SDB_TASK_HAS_CANCELED

from lib import testlib


class TestTask12495(testlib.SdbTestBase):
   def setUp(self):
      if testlib.is_standalone():
         self.skipTest("skip! This testcase do not support standlone")

      if testlib.get_data_groups().__len__() < 2:
         self.skipTest("only have signal group")

      l = testlib.get_data_groups()
      self.g1 = l[0]
      self.g2 = l[1]
      self.g1_name = self.g1["GroupName"]
      self.g2_name = self.g2["GroupName"]

      cl_option = {"ShardingKey": {"a": 1}, "ShardingType": "hash", "Group": self.g1_name}
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name, cl_option)

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)

   def test_task(self):
      list = [{"a": i} for i in range(10000)]
      self.cl.bulk_insert(0, list)
      id = self.cl.split_async_by_percent(self.g1_name, self.g2_name, 50.0)
      import sys
      if sys.version_info[0] != 3:
         id = long(id)
      self.db.list_tasks()
      try:
         self.db.cancel_task(task_id=id, is_async=True)
      except SDBError as e:
         if e.errcode != SDB_CAT_TASK_NOTFOUND or e.errcode != SDB_TASK_ALREADY_FINISHED:
            raise e
      try:
         self.db.wait_task([id], 1)
      except SDBError as e:
         if e.errcode != SDB_TASK_HAS_CANCELED:
            raise e

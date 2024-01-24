# -- coding: utf-8 --
# @decription: test split
# @testlink:   seqDB-12463
# @author:     LaoJingTang 2017-8-30

from lib import testlib
from pysequoiadb import client


class TestSplit12486(testlib.SdbTestBase):
   def setUp(self):
      self.db_list = []
      if testlib.is_standalone():
         self.skipTest("skip! This testcase do not support standlone")

      if testlib.get_data_groups().__len__() < 2:
         self.skipTest("only have signal group")

      l = testlib.get_data_groups()
      self.g1 = l[0]
      self.g2 = l[1]
      self.g1_name = self.g1["GroupName"]
      self.g2_name = self.g2["GroupName"]

   def get_cl_from_group_master(self, group_name):
      """
      connect to group master node and get CL
      :param group_name:
      :return: cl
      """
      m = self.db.get_replica_group_by_name(group_name=group_name).get_master()
      l = str(m).split(":")
      db = client(l[1].strip(), l[2].strip())
      self.db_list.append(db)
      return db.get_collection(self.cl_name_qualified)

   def _split_test(self, split_func, assert_func=None, insert_list=None, cl_option=None):
      # insert
      if insert_list is None:
         insert_list = [{"a": i} for i in range(100)]

      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name,options=cl_option)

      self.cl.bulk_insert(0, insert_list)
      # split
      split_func()

      # assert
      def assert_count():
         cl1 = self.get_cl_from_group_master(self.g1_name)
         cl2 = self.get_cl_from_group_master(self.g2_name)
         count = cl1.get_count() + cl2.get_count()
         self.assertEqual(100, count)

      if assert_func is None:
         assert_count()
      else:
         assert_func()
         assert_count()

   def assert_split_border(self, source_group_expect_list=None, target_group_expect_list=None):
      '''判断边界'''
      if source_group_expect_list is None:
         source_group_expect_list = [{"a": i} for i in range(50)]
      if target_group_expect_list is None:
         target_group_expect_list = [{"a": i} for i in range(50, 100)]
      cl1 = self.get_cl_from_group_master(self.g1_name)
      cl2 = self.get_cl_from_group_master(self.g2_name)
      r1 = testlib.get_all_records_noid(cl1.query());
      r2 = testlib.get_all_records_noid(cl2.query());
      self.assertListEqualUnordered(source_group_expect_list, r1)
      self.assertListEqualUnordered(target_group_expect_list, r2)

   def test_async_hash_percent(self):
      """
      测试用例 seqDB-12486 :: 版本: 1 :: 对分区表执行splitAsync进行百分比切分
      :return:
      """
      cl_option = {"ShardingKey": {"a": 1}, "ShardingType": "hash", "Group": self.g1_name}

      def split():
         id = self.cl.split_async_by_percent(self.g1_name, self.g2_name, 50.0)
         self.db.wait_task([id], 1)

      self._split_test(split, cl_option=cl_option)

   def test_async_range_condition(self):
      """
      测试用例 seqDB-12485 :: 版本: 1 :: 对分区表执行splitAsync进行范围切分
      :return:
      """
      cl_option = {"ShardingKey": {"a": 1}, "ShardingType": "range", "Group": self.g1_name}

      def split():
         id = self.cl.split_async_by_condition(self.g1_name, self.g2_name, {"a": 50}, {"a": 100})
         self.db.wait_task([id], 1)

      self._split_test(split, cl_option=cl_option, assert_func=self.assert_split_border)

   def test_split_range_percent(self):
      """
      测试用例 seqDB-12483 :: 版本: 1 :: 对分区表执行split进行百分比切分
      :return:
      """
      cl_option = {"ShardingKey": {"a": 1}, "ShardingType": "range", "Group": self.g1_name}

      def split():
         self.cl.split_by_percent(self.g1_name, self.g2_name, 50)

      self._split_test(split, cl_option=cl_option, assert_func=self.assert_split_border)

   def test_split_hash_range(self):
      """
       测试用例 seqDB-12482 :: 版本: 1 :: 对hash分区表执行split进行范围切分
      :return:
      """
      cl_option = {"ShardingKey": {"a": 1}, "ShardingType": "hash", "Group": self.g1_name}

      def split():
         self.cl.split_by_condition(self.g1_name, self.g2_name, {"a": 512}, {"a": 1024})

      self._split_test(split, cl_option=cl_option)

   def test_split_range_condition_two_shardingtype(self):
      """
      测试用例 seqDB-12484 :: 版本: 1 :: 分区表指定多个分区键执行split切分
      :return:
      """
      cl_option = {"ShardingKey": {"a": 1, "b": 1}, "ShardingType": "range", "Group": self.g1_name}
      insert_list = [{"a": i, "b": i} for i in range(100)]

      def split():
         self.cl.split_by_condition(self.g1_name, self.g2_name, {"a": 50, "b": 50}, {"a": 100, "b": 100})

      def assert_split():
         self.assert_split_border(insert_list[0:50], insert_list[50:100])

      self._split_test(split, cl_option=cl_option, insert_list=insert_list, assert_func=assert_split)

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)
      for x in self.db_list:
         x.disconnect()

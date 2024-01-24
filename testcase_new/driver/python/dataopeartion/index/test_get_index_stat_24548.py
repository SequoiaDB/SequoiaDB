# -*- coding: utf-8 -*-
# @decription: seqDB-24548:getIndexStat获取指定索引的统计信息
# @author:     liuli
# @createTime: 2021.11.02

from lib import testlib

insert_nums = 200
cs_name = "cs_24548"
cl_name = "cl_24548"
index_name = "index_24548"
class TestGetIndexStat24548(testlib.SdbTestBase):
   def setUp(self):
      testlib.drop_cs(self.db, cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(cs_name)
      self.cl = self.cs.create_collection(cl_name)

   def test_get_index_stat_24548(self):
      # 插入数据并创建索引
      for i in range(insert_nums):
         self.cl.insert({"_id": i, "a": i, "b": "test" + str(i)})
      self.cl.create_index_with_option({"a":1},index_name)
      # 收集对应的统计信息
      self.db.analyze({"Collection":cs_name+"."+cl_name,"Index":index_name})
      # 获取指定的索引统计信息并校验
      actResult = self.cl.get_index_stat(index_name)
      del actResult["StatTimestamp"]
      expResult = {"Collection": cs_name + "." + cl_name, "Index": index_name, "Unique": False, "KeyPattern": {"a": 1},
                   "TotalIndexLevels": 1, "TotalIndexPages": 1, "DistinctValNum": [200], "MinValue": {"a": 0},
                   "MaxValue": {"a": 199}, "NullFrac": 0, "UndefFrac": 0, "SampleRecords": 200, "TotalRecords": 200}
      self.assertEqual(len(expResult), len(actResult),
                       "actResult : " + str(actResult) + ", expResult : " + str(expResult))
      for key in actResult:
         self.assertEqual(expResult[key], actResult[key],
                       "actResult : " + str(actResult) + ", expResult : " + str(expResult))

   def tearDown(self):
      testlib.drop_cs(self.db, cs_name, ignore_not_exist=True)
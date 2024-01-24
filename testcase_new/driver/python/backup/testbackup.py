# -- coding: utf-8 --
# @decription: test backup
# @testlink:  测试用例 seqDB-12494 :: 版本: 1 :: 备份/列出备份/删除备份全部数据库信息
#             测试用例 seqDB-12493 :: 版本: 1 :: 备份/列出备份/删除备份指定组的数据库
# @author:     LaoJingTang 2017-8-30
from lib import testlib


class SdbTestBackup(testlib.SdbTestBase):

   def test_backup_12494(self):
      self.db.remove_backup({"Name": "mybk"})
      self.db.backup({"Name": "mybk"})
      cur = self.db.list_backup({})
      l = testlib.get_all_records_noid(cur)
      names = []
      for x in l:
         names.append(x["Name"])
      self.assertIn("mybk", names)

      cur = self.db.list_backup({"Name": "mybk"})
      l = testlib.get_all_records_noid(cur)
      names = []
      for x in l:
         names.append(x["Name"])
      self.assertIn("mybk", names)

      self.db.remove_backup({"Name": "mybk"})

   def test_backup_12493(self):
      if testlib.is_standalone():
         self.skipTest("not support standlone mode")
      self.db.remove_backup({"Name": "mybk"})
      group_name=testlib.get_data_groups()[0]["GroupName"]
      self.db.backup({"Name": "mybk","GroupName":[group_name]})
      cur = self.db.list_backup({"Name": "mybk1"})
      l = testlib.get_all_records_noid(cur)
      for x in l:
         self.assertEqual(x["GroupName"],group_name)

      self.db.remove_backup({"Name":"mybk"})



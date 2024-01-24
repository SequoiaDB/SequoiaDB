# -- coding: utf-8 --
"""
 @decription:
 @testlink:   seqDB-12468 :: 版本: 1 :: truncate所有记录
 @author:     laojingtang
"""
from lib import testlib


class SdbTestTruncate12468(testlib.SdbTestBase):
   def setUp(self):
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)

   def test_truncate(self):
      cl = self.cl
      cl.bulk_insert(0, [{"a": i} for i in range(10)])
      cl.truncate()
      r = testlib.get_all_records_noid(cl.query())
      self.assertEqual(0, len(r))

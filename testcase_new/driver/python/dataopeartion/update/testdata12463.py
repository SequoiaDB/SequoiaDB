# @decription: data  opeartion
# @testlink:   seqDB-12463
# @author:     LaoJingTang 2017-8-30

from lib import testlib


class Data12463Sdb(testlib.SdbTestBase):
   def setUp(self):
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)

   # condition+update
   def test12463(self):
      self.original_list = [{"a": 1} for i in range(10)]
      self.cl.bulk_insert(1, self.original_list)
      update = {"$inc": {"a": 1}}
      condition = {"a": {"$et": 1}}
      self.cl.update(update, condition=condition)
      l = [{"a": 2} for i in range(10)]
      self.assertListEqualUnordered(l, testlib.get_all_records_noid(self.cl.query()))

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)

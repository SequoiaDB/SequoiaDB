# @decription: lob opeartion
# @testlink:   seqDB-12478
# @author:     LaoJingTang 2017-8-30

from bson.objectid import ObjectId
from lib import testlib
from lob import util


class Lob12478(testlib.SdbTestBase):
   def setUp(self):
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)

   def test_lob(self):
      self.lob_size = 1024
      self.lob_context = util.random_str(self.lob_size)
      self.md5 = util.get_md5(self.lob_context)

      self.create_lobs_test(10)
      self.read_lobs_test()
      self.del_lob_test()

   def create_lobs_test(self, num=10):
      self.oid_set = set()
      # not specify oid
      for i in range(num):
         lob = self.cl.create_lob()
         lob.write(self.lob_context, len(self.lob_context))
         oid = lob.get_oid()
         self.oid_set.add(oid)
         lob.close()

      # specify oid
      for i in range(num):
         oid = ObjectId()
         lob = self.cl.create_lob(oid)
         self.oid_set.add(oid)
         lob.write(self.lob_context, len(self.lob_context))
         lob.close()

   def read_lobs_test(self):
      for oid in self.oid_set:
         lob = self.cl.get_lob(oid)
         context = lob.read(lob.get_size())
         self.check_lob_md5(lob, context)

         lob.seek(0)
         context = lob.read(lob.get_size())
         self.check_lob_md5(lob, context)

         lob.seek(lob.get_size(), 2)
         context = lob.read(lob.get_size())
         self.check_lob_md5(lob, context)

         lob.seek(0)
         size = int(lob.get_size() / 2)
         lob.seek(size, 1)
         context = lob.read(lob.get_size() - 1)
         if isinstance(context, str):
            pass
         else:
            context = context.decode('utf-8')

         if context not in self.lob_context:
            self.fail("seek context not in lob_context")
         lob.close()

   def del_lob_test(self):
      for oid in self.oid_set:
         self.cl.remove_lob(oid)
      cr = self.cl.list_lobs()
      count = testlib.get_all_records(cr).__len__()
      if count != 0:
         self.fail("not remove all lob ")

   def check_lob_md5(self, lob, context):
      if util.get_md5(context) != self.md5:
         self.fail("lobid: " + lob.get_oid() + " except md5: " + self.md5)
      if lob.get_size() != self.lob_size:
         self.fail(
            "lobid: " + str(lob.get_oid()) + " except size: " + str(self.lob_size) + "actually: " + str(lob.get_size()))

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)

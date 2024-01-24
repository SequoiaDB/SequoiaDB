# -- coding: utf-8 --
# @decription: truncate lob test
# @testlink:   seqDB-12478
# @author:     LaoJingTang 2017-11-30
from lib import testlib
from lob import util

class TestTruncateLob(testlib.SdbTestBase):
   @classmethod
   def setUpClass(cls):
      testlib.SdbTestBase.setUpClass()
      cls.db.drop_cs_if_exist(cls.cs_name)
      cls.cs = cls.db.create_collection_space(cls.cs_name)
      cls.data = util.random_str(1024)
      cls.expect_data = cls.data.encode()

   @classmethod
   def tearDownClass(cls):
      cls.db.drop_collection_space(cls.cs_name)
      testlib.SdbTestBase.tearDownClass()

   def setUp(self):
      self.dbcl = self.__createSimpleCl()

   def tearDown(self):
      self.cs.drop_collection(self.cl_name)

   def test13388_13390(self):
      for i in [1024, 1, 512, 1025]:
         self.__dropAllLobs()
         oid = self.createAndWriteLob(data=self.data)
         self.dbcl.truncate_lob(oid=oid, length=i)

         actual = self.__readLob(oid)
         expect = self.expect_data[0:i]
         self.assertEqual(actual, expect)

         lobs_info = testlib.get_all_records(self.dbcl.list_lobs())
         actual_lob_size = lobs_info[0]["Size"]
         if i > len(self.data):
            expect_lob_size = len(self.data)
         else:
            expect_lob_size = i
         self.assertEqual(actual_lob_size, expect_lob_size)

   def __dropAllLobs(self):
      self.cs.drop_collection(self.cl_name)
      self.__createSimpleCl()

   def __createSimpleCl(self):
      return self.cs.create_collection(self.cl_name)

   def createAndWriteLob(self, data, oid=None):
      lob = self.dbcl.create_lob(oid)
      lob.write(data, len(data))
      lob.close()
      return lob.get_oid()

   def __readLob(self, oid):
      lob = self.dbcl.open_lob(oid)
      length = lob.get_size()
      return lob.read(length=length)

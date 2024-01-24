# @decription: find records and interrupt
# @testlink:   seqDB-20136
# @interface:  interrupt(self)
# @author:     liuxiaoxuan 2019-10-31

from lib import testlib
from pysequoiadb.error import (SDBBaseError, SDBEndOfCursor)

class TestInterrupt20136(testlib.SdbTestBase):
   def setUp(self):
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)
      self.insert_datas(10)

   def test_interrupt_20136(self):

      # query all records and interrupt
      cursor = self.cl.query()
      cursor.next()
      self.db.interrupt()
      try:
         cursor.next()
         self.fail("need throw error")
      except SDBBaseError as e:
         self.assertEqual(-31, e.code)
      finally:
         cursor.close()

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)

   def insert_datas(self, insert_nums):
      doc = []
      for i in range(0, insert_nums):
         doc.append({"_id": i, "a": "test" + str(i)})
      try:
         flags = 0
         self.cl.bulk_insert(flags, doc)
      except SDBBaseError as e:
         self.fail('insert fail: ' + str(e))

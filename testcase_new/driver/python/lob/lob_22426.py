# -*- coding: utf-8 -*-

# @testlink   seqDB-22426
# @interface  cl.open_lob() by mode ' LOB_SHARE_READ '&' LOB_SHARE_READ|LOB_WRITE'
# @author     Zixian Yan 2020-07-30

from lob import util
from lib import testlib
from pysequoiadb import lob
from pysequoiadb.lob import LOB_WRITE
from pysequoiadb.lob import LOB_SHARE_READ
from pysequoiadb.error import SDBBaseError

LOB_LENGTH = 1024

class Lob_22426(testlib.SdbTestBase):

   def setUp(self):
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)

   def test_main(self):
       self.create_a_lob()
       # In LOB_SHARED_READ mode. Excute 'read' operation --- Task 1
       self.read_operation(LOB_SHARE_READ)
        # In LOB_SHARED_READ_AND_WRITE mode. Excute 'read' &'write' operations --- Task 2
       self.write_operation(LOB_SHARE_READ | LOB_WRITE)
       self.read_operation( LOB_SHARE_READ | LOB_WRITE)

   def create_a_lob(self):
       self.data_size = LOB_LENGTH
       self.lob_content = util.random_str(self.data_size)
       lob = self.cl.create_lob()
       lob_length = len(self.lob_content)
       lob.write(self.lob_content, lob_length)
       self.lob_oid = lob.get_oid()
       lob.close()

   def read_operation(self, mode):
       oid = self.lob_oid
       lob = self.cl.open_lob( oid, mode )
       try:
           content = lob.read(LOB_LENGTH)
       except SDBBaseError as error:
           print("Failed to read data of 'Lob' obejct.\nError Message: " + error)

       #Convert 'bytes' type to 'string' type, Only for python3
       if type(content) is bytes:
           content = content.decode()
       #Checkout if read_content and lob_content match or not.
       expect_content = self.lob_content
       self.assertEqual(expect_content, content)
       lob.close()

   def write_operation(self, mode):
       oid = self.lob_oid
       lob = self.cl.open_lob( oid, mode )
       try:
           new_data = util.random_str(LOB_LENGTH)
           lob.write(new_data, LOB_LENGTH)
       except SDBBaseError as error:
           print("Failed to write data of 'Lob' obejct.\nError Message: " + error)
       self.lob_content = new_data
       lob.close()

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)

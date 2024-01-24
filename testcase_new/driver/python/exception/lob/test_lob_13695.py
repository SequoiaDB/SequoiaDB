# @decription: get lob/delete lob fail
# @testlink:   seqDB-13695
# @author:     liuxiaoxuan 2017-12-07

from lib import testlib
from pysequoiadb.error import (SDBBaseError, SDBIOError, SDBInvalidArgument)

class LobException13695(testlib.SdbTestBase):
   def setUp(self):
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)

   def test_lob_13695(self):
      # check lob non exist
      oid_not_exist = "5a28fbe2e79c27700a000001"
      self.check_io_error_read_lob(oid_not_exist)
      self.check_io_error_delete_lob(oid_not_exist)
      
      # check lob invalid
      oid_not_invalid = "500000000001"
      self.check_invalid_error_read_lob(oid_not_invalid)
      self.check_invalid_error_delete_lob(oid_not_invalid)
      
   def check_io_error_read_lob(self, oid):
      try:
         self.cl.get_lob(oid)
         self.fail("NEED IO ERROR")
      except SDBIOError as e:
         self.assertEqual(e.code, -4)
         self.assertEqual(e.detail, "Failed to get specified lob")

   def check_io_error_delete_lob(self, oid):
      try:
         self.cl.remove_lob(oid)
         self.fail("NEED IO ERROR")
      except SDBIOError as e:
         self.assertEqual(e.code, -4)
         self.assertEqual(e.detail, "Failed to remove lob")  

   def check_invalid_error_read_lob(self, oid):
      try:
         self.cl.get_lob(oid)
         self.fail("NEED INVALID ERROR")
      except SDBInvalidArgument as e:
         self.assertEqual(e.code, -6)
         self.assertEqual(e.detail, "invalid oid: " + "'" + oid + "'")

   def check_invalid_error_delete_lob(self, oid):
      try:
         self.cl.remove_lob(oid)
         self.fail("NEED INVALID ERROR")
      except SDBInvalidArgument as e:
         self.assertEqual(e.code, -6)
         self.assertEqual(e.detail, "invalid oid: " + "'" + oid + "'")          

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)

# @decription: create exist cs/cl, get/remove non exist cs/cl
# @testlink:   seqDB-13677
# @author:     liuxiaoxuan 2017-12-07

from lib import testlib
from pysequoiadb.error import (SDBBaseError, SDBError)

class CsClException13677(testlib.SdbTestBase):
   def setUp(self):
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)
      cl_option = {'ReplSize' : 0}
      self.cs.create_collection(self.cl_name, cl_option)

   def test_cs_cl_13677(self):
      # check create cs and cl success
      self.check_create_cs_cl_success(self.cs_name, self.cl_name)
      
      # check create exist cs and cl
      self.check_error_create_cs(self.cs_name)
      self.check_error_create_cl(self.cl_name)
      
      # drop exist cl 
      self.cs.drop_collection(self.cl_name)
      
      # check drop cl success
      self.check_drop_cl_success(self.cs_name, self.cs_name)
      
      # get non exist cl
      self.check_error_get_cl(self.cl_name)
  
      # drop non exist cl
      self.check_error_drop_cl(self.cl_name)
      
      # drop exist cs
      self.db.drop_collection_space(self.cs_name)
      
      # check drop cs success
      self.check_drop_cs_success(self.cs_name)
      
      # get non exist cs
      self.check_error_get_cs(self.cs_name)
  
      # drop non exist cs
      self.check_error_drop_cs(self.cs_name)
      
   def check_create_cs_cl_success(self, cs_name, cl_name):
      try:
         cursor1 = self.db.list_collection_spaces()
         act_cs = testlib.get_all_records_noid(cursor1)
         
         cursor2 = self.db.list_collections()
         act_cl = testlib.get_all_records_noid(cursor2)
         
         # check cs
         expect_cs = {'Name': cs_name}
         self.assertIn(expect_cs, act_cs)
            
         # check cl
         expect_cl = {'Name': cs_name + "." + cl_name}
         self.assertIn(expect_cl, act_cl)
            
      except SDBBaseError as e:
         self.fail("check create cs or cl fail: " + str(e))
         
   def check_drop_cs_success(self, cs_name):
      try:
         cursor = self.db.list_collection_spaces()
         act_cs = testlib.get_all_records_noid(cursor)
         
         # check cs
         expect_cs = {'Name': cs_name}
         self.assertNotIn(expect_cs, act_cs)
      except SDBBaseError as e:
         self.fail("check drop cs fail: " + str(e))
         
   def check_drop_cl_success(self, cs_name, cl_name):
      try:
         cursor = self.db.list_collections()
         act_cl = testlib.get_all_records_noid(cursor)
         
         # check cl
         expect_cl = {'Name': cs_name + "." + cl_name}
         self.assertNotIn(expect_cl, act_cl)
      except SDBBaseError as e:
         self.fail("check drop cl fail: " + str(e))
         
   def check_error_create_cs(self, cs_name):
      try:
         self.db.create_collection_space(cs_name)
         self.fail("NEED SDB ERROR")
      except SDBError as e:
         self.assertEqual(e.code, -33)
         self.assertEqual(e.detail, "Failed to create collection space: " + cs_name) 
         
   def check_error_create_cl(self, cl_name):
      try:
         self.cs.create_collection(cl_name)
         self.fail("NEED SDB ERROR")
      except SDBError as e:
         self.assertEqual(e.code, -22)
         self.assertEqual(e.detail, "Failed to create collection") 
         
   def check_error_get_cs(self, cs_name):
      try:
         self.db.get_collection_space(cs_name)
         self.fail("NEED SDB ERROR")
      except SDBError as e:
         self.assertEqual(e.code, -34)
         self.assertEqual(e.detail, "Failed to get collection space: " + cs_name)  

   def check_error_drop_cs(self, cs_name):
      try:
         self.db.drop_collection_space(cs_name)
         self.fail("NEED SDB ERROR")
      except SDBError as e:
         self.assertEqual(e.code, -34)
         self.assertEqual(e.detail, "Failed to drop collection space: " + cs_name)               
     
   def check_error_get_cl(self, cl_name):
      try:
         self.cs.get_collection(cl_name)
         self.fail("NEED SDB ERROR")
      except SDBError as e:
         self.assertEqual(e.code, -23)
         self.assertEqual(e.detail, "Failed to get collection: " + cl_name)   
         
   def check_error_drop_cl(self, cl_name):
      try:
         self.cs.drop_collection(cl_name)
         self.fail("NEED SDB ERROR")
      except SDBError as e:
         self.assertEqual(e.code, -23)
         self.assertEqual(e.detail, "Failed to drop collection")   

   def tearDown(self):
      if self.should_clean_env():
         try:
            self.db.drop_collection_space(self.cs_name)
         except SDBBaseError as e:
            if -34 != e.code:
               self.fail("teardown fail: " + e.detail) 

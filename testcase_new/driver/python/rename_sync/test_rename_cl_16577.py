# @decription rename collection and metadata operation
# @testlink   seqDB-16577
# @interface  rename_collection	( self, old_name, new_name, options = None )		
# @author     yinzhen 2018-12-06

from lib import testlib
from lib import sdbconfig
from pysequoiadb.error import (SDBBaseError, SDBEndOfCursor)

class TestRenameCL16577(testlib.SdbTestBase):
   def setUp(self):
      # create cs 
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)      
      self.cs = self.db.create_collection_space(self.cs_name)

   def test_rename_cs_16577(self):
      #create collection
      cl_name = "test_cl_16577"
      self.cl = self.cs.create_collection(cl_name)
      self.assertTrue(self.is_collection_exist(cl_name))
      
      #rename collection
      cl_new_name = "test_cl_new_16577"
      self.cs.rename_collection(cl_name, cl_new_name)
      self.assertTrue(self.is_collection_exist(cl_new_name))
      self.assertFalse(self.is_collection_exist(cl_name))
      
      #insert
      self.cl = self.cs.get_collection(cl_new_name)
      self.insert_records()
      cursor = self.cl.query()
      act_result = testlib.get_all_records_noid(cursor)
      exp_result = self.get_query_expect_records()
      msg = str(exp_result) + "expect is not equal to actResult" + str(act_result)
      self.assertListEqual(act_result, exp_result, msg)
      
      #update
      self.cl.update({"$set":{"a":"a999"}}, condition = {"a":"a0"})
      cursor = self.cl.query()
      act_result = testlib.get_all_records_noid(cursor)
      exp_result = self.get_update_expect_records()
      msg = str(exp_result) + "expect is not equal to actResult" + str(act_result)
      self.assertListEqual(act_result, exp_result, msg)
      
      #delete
      self.cl.delete(condition = {"a":"a999"})
      cursor = self.cl.query()
      act_result = testlib.get_all_records_noid(cursor)
      exp_result = self.get_delete_expect_records()
      msg = str(exp_result) + "expect is not equal to actResult" + str(act_result)
      self.assertListEqual(act_result, exp_result, msg)
      
      #rename not exist collection
      try:
         self.cs.rename_collection(cl_name, "test2_16577")
      except SDBBaseError as e:
         self.assertEqual(-23, e.code, e.detail)
      self.assertFalse(self.is_collection_exist("test2_16577"))
      
   def tearDown(self):
      self.db.drop_collection_space(self.cs_name)   
      self.db.disconnect()
      
   def insert_records(self):
      doc = []
      for i in range(0, 10):
         doc.append({"a":"a"+str(i)})
      try:
         flags = 0
         self.cl.bulk_insert(flags, doc)
      except SDBBaseError as e:
         self.fail('insert fail: ' + str(e))
      
   def is_collection_exist(self, collection_name):
      try:
         cl_name = self.cs.get_collection(collection_name).get_collection_name()
         if (cl_name == collection_name):
            return True
      except SDBBaseError as e:
         self.assertEqual(-23, e.code, e.detail)
         return False
         
   def get_query_expect_records(self):
      doc = []
      for i in range(0, 10):
         doc.append({"a":"a"+str(i)})
      return doc
   
   def get_update_expect_records(self):
      doc = []
      doc.append({"a":"a999"})
      for i in range(1, 10):
         doc.append({"a":"a"+str(i)})
      return doc
      
   def get_delete_expect_records(self):
      doc = []
      for i in range(1, 10):
         doc.append({"a":"a"+str(i)})
      return doc
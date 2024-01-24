# @decription: insert record include oid
# @testlink:   seqDB-12461
# @interface:  bulk_insert(flag,record) ,flag set 0/1/2,record set list/tuple
# @author:     zhaoyu 2017-8-30

from dataopeartion.insert.commlib import *
from lib import testlib
from pysequoiadb.error import SDBBaseError

class TestInsert12461(testlib.SdbTestBase):
   def setUp(self):
      self.db.set_session_attri({"PreferedInstance": "M"})
      
      #create cs and cl
      self.cs_name = "cs_12461"
      self.cl_name = "cl_12461"
      try:
         self.db.drop_collection_space(self.cs_name)
      except SDBBaseError as e:
         if -34 != e.code:
            self.fail("drop_cs_fail,detail:" + str(e))
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)
      
   def test_insert_12461(self):
      #insert list data
      record = [{"a": 1}, {"a": 2}]
      self.cl.bulk_insert(0, record)
      
      #query data and check
      check_Result(self.cl, {}, record, False)
      
      #delete data
      self.cl.delete()
      
      #insert tuple data
      record = ({"a": 1}, {"a": 2})
      self.cl.bulk_insert(1, record)
      
      #query data and check
      check_Result(self.cl, {}, record, False)
      
      #delete data
      self.cl.delete()
      
      #check flag set 0
      record = [{"a": 1, "_id": 1}, {"a": 2, "_id": 1}, {"a": 3, "_id": 2}]
      try:
         self.cl.bulk_insert(0, record)
         self.fail("need_an_error")
      except SDBBaseError as e:
         if -38 != e.code:
            self.fail("check_error_code_fail,detail:" + str(e))
      
      #query data and check
      expect_record = [{"a": 1, "_id": 1}]
      check_Result(self.cl, {}, expect_record, True)
      
      #delete data
      self.cl.delete()
      
      #check flag set 1
      record = [{"a": 1, "_id": 1}, {"a": 2, "_id": 1}, {"a": 3, "_id": 2}]
      self.cl.bulk_insert(1, record)
      
      #query data and check
      expect_record = [{"a": 1, "_id": 1}, {"a": 3, "_id": 2}]
      check_Result(self.cl, {}, expect_record, True)
      
      #delete data
      self.cl.delete()
      
      #check flag set 2
      try:
         self.cl.bulk_insert(2, record)
         self.fail("need_an_error")
      except SDBBaseError as e:
         if -38 != e.code:
            self.fail("check_error_code_fail,detail:" + str(e))
      
      #query data and check
      expect_record = [{"a": 1, "_id": 1}]
      check_Result(self.cl, {}, expect_record, True)
      
   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)
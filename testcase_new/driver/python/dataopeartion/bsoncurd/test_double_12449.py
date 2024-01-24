# @decription: insert float data
# @testlink:   seqDB-12448
# @interface:  insert(record)
#              update(rule, kwargs)
#              delete(kwargs)
# @author:     zhaoyu 2017-8-31

from pysequoiadb.error import (SDBBaseError)
from dataopeartion.bsoncurd.commlib import *
from lib import testlib

class TestDouble12449(testlib.SdbTestBase):
   def setUp(self):
      # create cs and cl
      self.cs_name = "cs_12449"
      self.cl_name = "cl_12449"
      try:
         self.db.drop_collection_space(self.cs_name)
      except SDBBaseError as e:
         if -34 != e.code:
            self.fail("drop_cs_fail,detail:" + str(e))
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)
      
   def test_double_12449(self):
      #insert int data
      data1 = -1.7E+308
      data2 = 1.7E+308
      data3 = -4.9E-324
      data4 = 4.9E-324
      record = [{"a": data1}, {"a": data2}, {"a": data3}, {"a": data4}]
      self.cl.bulk_insert(0, record)
      
      #query data and check
      expect_type = [{"a": "double"}, {"a": "double"}, {"a": "double"}, {"a": "double"}]
      check_Result(self.cl, {}, {"a":{"$type":2}}, record, expect_type, False)
      
      #update data
      data1_after_update = 1
      data2_after_update = -1
      data3_after_update = 0.25
      data4_after_update = -0.25
      self.cl.update({"$set": {"a": data1_after_update}}, condition={"a": data1})
      self.cl.update({"$set": {"a": data2_after_update}}, condition={"a": data2})
      self.cl.update({"$set": {"a": data3_after_update}}, condition={"a": data3})
      self.cl.update({"$set": {"a": data4_after_update}}, condition={"a": data4})
      
      #query data and check
      expect_type_after_update = [{"a": "int32"}, {"a": "int32"}, {"a": "double"}, {"a": "double"}]
      record_after_update = [{"a": data1_after_update}, {"a": data2_after_update},
                             {"a": data3_after_update}, {"a": data4_after_update}]
      check_Result(self.cl, {}, {"a": {"$type": 2}}, record_after_update, expect_type_after_update, False)
      
      #update data
      self.cl.update({"$set": {"a": data1}},condition={"a": data1_after_update})
      self.cl.update({"$set": {"a": data2}},condition={"a": data2_after_update})
      self.cl.update({"$set": {"a": data3}},condition={"a": data3_after_update})
      self.cl.update({"$set": {"a": data4}},condition={"a": data4_after_update})
      
      #query data and check
      check_Result(self.cl, {}, {"a": {"$type": 2}}, record, expect_type, False)
      
      #delete data
      self.cl.delete()
      
      #query data and check
      check_Result(self.cl, {}, {}, {}, {}, False)
      
      #insert int data 
      self.cl.bulk_insert(0, record)
      
      #query data and check
      check_Result(self.cl, {}, {"a": {"$type": 2}}, record, expect_type, False)
      
      #delete data
      self.cl.delete(condition={"a": data1})
      self.cl.delete(condition={"a": {"$et": data2}})
      self.cl.delete(condition={"a": data3})
      self.cl.delete(condition={"a": {"$et": data4}})
      
      #query data and check
      check_Result(self.cl, {}, {}, {}, {}, False)
      
      #insert out of range int
      data5 = -1.7E+309
      data6 = 1.7E+309
      data7 = -4.9E-325
      data8 = 4.9E-325 
      record = [{"a": data5}, {"a": data6}, {"a": data7}, {"a": data8}]
      self.cl.bulk_insert(0, record)
      
      #query data and check
      expect_type = [{"a": "double"},{"a": "double"}, {"a": "double"},{"a": "double"}]
      check_Result(self.cl, {}, {"a":{"$type":2}}, record, expect_type, False)

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)
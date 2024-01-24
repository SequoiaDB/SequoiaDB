# @decription: insert oid data
# @testlink:   seqDB-12451
# @interface:  insert(record)
#              update(rule, kwargs)
#              delete(kwargs)
# @author:     zhaoyu 2017-8-31

import datetime
from pysequoiadb.error import (SDBBaseError)
from bson.objectid import ObjectId
from dataopeartion.bsoncurd.commlib import *
from lib import testlib

from bson.json_util import loads 
from bson.json_util import dumps

class TestOid12451(testlib.SdbTestBase):
   def setUp(self):
      # create cs and cl
      self.cs_name = "cs_12451"
      self.cl_name = "cl_12451"
      try:
         self.db.drop_collection_space(self.cs_name)
      except SDBBaseError as e:
         if -34 != e.code:
            self.fail("drop_cs_fail,detail:" + str(e))
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)

   def test_oid_12451(self):
      #insert data
      data1 = ObjectId()
      data2 = ObjectId.from_datetime(datetime.datetime(1902, 1, 1))
      data3 = ObjectId.from_datetime(datetime.datetime(2037, 12, 31))
      record = [{"a": data1}, {"a": data2}, {"a": data3}]
      self.cl.bulk_insert(0, record)
      
      #query data and check
      expect_type = [{"a": "oid"}, {"a": "oid"}, {"a": "oid"}]
      check_Result(self.cl, {}, {"a": {"$type": 2}}, record, expect_type, False)
      
      #update data
      data1_after_update = 1
      data2_after_update = 0
      data3_after_update = -1
      self.cl.update({"$set": {"a": data1_after_update}}, condition={"a": data1})
      self.cl.update({"$set": {"a": data2_after_update}}, condition={"a": data2})
      self.cl.update({"$set": {"a": data3_after_update}}, condition={"a": data3})
      
      #query data and check
      expect_type_after_update = [{"a": "int32"}, {"a": "int32"}, {"a": "int32"}]
      record_after_update = [{"a": data1_after_update},{"a": data2_after_update}, {"a": data3_after_update}]
      check_Result(self.cl, {}, {"a": {"$type": 2}}, record_after_update, expect_type_after_update, False)
      
      #update data
      self.cl.update({"$set": {"a": data1}}, condition={"a": data1_after_update})
      self.cl.update({"$set": {"a": data2}}, condition={"a": data2_after_update})
      self.cl.update({"$set": {"a": data3}}, condition={"a": data3_after_update})
      
      #query data and check
      check_Result(self.cl, {}, {"a": {"$type": 2}}, record, expect_type, False)
      
      #delete data
      self.cl.delete()
      
      #query data and check
      check_Result(self.cl, {}, {}, {}, {}, False)
      
      #insert data 
      self.cl.bulk_insert(0, record)
      
      #query data and check
      check_Result(self.cl, {}, {"a": {"$type": 2}}, record, expect_type, False)
      
      #delete data
      self.cl.delete(condition={"a": data1})
      self.cl.delete(condition={"a": {"$et": data2}})
      self.cl.delete(condition={"a": {"$et": data3}})
      
      #query data and check
      check_Result(self.cl, {}, {}, {}, {}, False)
      
      """
      #timestamp out of range
      try:
         data4 = ObjectId.from_datetime(datetime.datetime(1901,1,1))
         self.fail("NEED_AN_ERROR")
      except SDBBaseError as e:
         print(e.detail)
      """
      
      #is_valid()
      if(not ObjectId.is_valid(ObjectId())):
         self.fail("check_is_valid_fail")
         
      if(ObjectId.is_valid("abc")):
         self.fail("check_is_valid_fail")
      
      #json to bson   
      json = '{"$oid": "59a7bfd087310ecb73000009"}'
      self.assertEqual(json, dumps(loads(json)))

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)
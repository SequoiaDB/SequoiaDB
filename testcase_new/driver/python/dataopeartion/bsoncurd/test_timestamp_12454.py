# @decription: insert date data
# @testlink:   seqDB-12454
# @interface:  insert(record)
#              update(rule, kwargs)
#              delete(kwargs)
# @author:     zhaoyu 2017-9-1

import unittest
from pysequoiadb.error import (SDBBaseError)
import datetime
from bson.timestamp import Timestamp
from dataopeartion.bsoncurd.commlib import *
from lib import testlib

from bson.json_util import loads 
from bson.json_util import dumps

class TestTimestamp12454(testlib.SdbTestBase):
   def setUp(self):
      # create cs and cl
      self.cs_name = "cs_12454"
      self.cl_name = "cl_12454"
      try:
         self.db.drop_collection_space(self.cs_name)
      except SDBBaseError as e:
         if (-34 != e.code):
            self.fail("drop_cs_fail:" + str(e))
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)
      
   def test_timestamp_12454(self):
      #insert data
      data1 = '{"$timestamp":"1902-01-01-00.00.00.000000"}'
      data2 = '{"$timestamp":"2037-12-31-23.59.59.999999"}'
      data3 = Timestamp(datetime.datetime(1902, 1, 1, 0, 0), 0)
      data4 = Timestamp(datetime.datetime(2037, 12, 31, 23, 59), 59)
      data5 = Timestamp(-2147483648, 2147483647)
      data6 = Timestamp(2147483647, -2147483648)
      record = [{"a": loads(data1)}, {"a": loads(data2)}, {"a": data3}, {"a": data4}, {"a": data5}, {"a": data6}]
      self.cl.bulk_insert(0, record)
      
      #query data and check
      expect_type = [{"a": "timestamp"}, {"a": "timestamp"}, {"a": "timestamp"}, {"a": "timestamp"}, {"a": "timestamp"}, {"a": "timestamp"}]
      check_Result(self.cl, {}, {"a": {"$type": 2}}, record, expect_type, False)
      
      #update data
      data1_after_update = 1
      data2_after_update = 0
      data3_after_update = -1
      data4_after_update = -2
      data5_after_update = -3
      data6_after_update = -4
      self.cl.update({"$set": {"a": data1_after_update}}, condition={"a": loads(data1)})
      self.cl.update({"$set": {"a": data2_after_update}}, condition={"a": loads(data2)})
      self.cl.update({"$set": {"a": data3_after_update}}, condition={"a": data3})
      self.cl.update({"$set": {"a": data4_after_update}}, condition={"a": data4})
      self.cl.update({"$set": {"a": data5_after_update}}, condition={"a": data5})
      self.cl.update({"$set": {"a": data6_after_update}}, condition={"a": data6})
      
      #query data and check
      expect_type_after_update = [{"a": "int32"}, {"a": "int32"}, {"a": "int32"}, {"a": "int32"}, {"a": "int32"}, {"a": "int32"}]
      record_after_update = [{"a": data1_after_update}, {"a": data2_after_update}, {"a": data3_after_update}, {"a": data4_after_update}, {"a": data5_after_update}, {"a": data6_after_update}]
      check_Result(self.cl, {}, {"a": {"$type": 2}}, record_after_update, expect_type_after_update, False)
      
      #update data
      self.cl.update({"$set": {"a": loads(data1)}}, condition={"a": data1_after_update})
      self.cl.update({"$set": {"a": loads(data2)}}, condition={"a": data2_after_update})
      self.cl.update({"$set": {"a": data3}}, condition={"a": data3_after_update})
      self.cl.update({"$set": {"a": data4}}, condition={"a": data4_after_update})
      self.cl.update({"$set": {"a": data5}}, condition={"a": data5_after_update})
      self.cl.update({"$set": {"a": data6}}, condition={"a": data6_after_update})
      
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
      self.cl.delete(condition={"a": loads(data1)})
      self.cl.delete(condition={"a": {"$et": loads(data2)}})
      self.cl.delete(condition={"a": {"$et": data3}})
      self.cl.delete(condition={"a": {"$et": data4}})
      self.cl.delete(condition={"a": {"$et": data5}})
      self.cl.delete(condition={"a": {"$et": data6}})
      
      #query data and check
      check_Result(self.cl, {}, {}, {}, {}, False)
      
      #json to bson   
      json = '{"$timestamp": "2012-01-01-13.14.26.123456"}'
      self.assertEqual(json, dumps(loads(json)))
      
      #time
      time_data3 = data3.time
      self.assertEqual(time_data3, -2145916800)
      time_data4 = data4.time
      self.assertEqual(time_data4, 2145916740)
      time_data5 = data5.time
      self.assertEqual(time_data5, -2147481501)
      time_data6 = data6.time
      self.assertEqual(time_data6, 2147481499)
      
      #inc
      inc_data3 = data3.inc
      self.assertEqual(inc_data3, 0)
      inc_data4 = data4.inc
      self.assertEqual(inc_data4, 59)
      inc_data5 = data5.inc
      self.assertEqual(inc_data5, 483647)
      inc_data6 = data6.inc
      self.assertEqual(inc_data6, 516352)
      
   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)
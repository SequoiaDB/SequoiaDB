# @decription: nuberLong data
# @testlink:   seqDB-9461/seqDB-9460/seqDB-7825/seqDB-7826/seqDB-7827
# @interface:  insert(record)
#              update(rule, kwargs)
#              delete(kwargs)
# @author:     Ting YU 2016-8-23/zhaoyu modify 2017-9-7

from pysequoiadb.error import (SDBBaseError)
from lib import testlib
from bson.json_util import loads
from bson.json_util import dumps
from dataopeartion.bsoncurd.commlib import *
import bson

class TestNumberLong9460(testlib.SdbTestBase):
   def setUp(self):
      # create cs and cl
      self.cs_name = "cs_9460"
      self.cl_name = "cl_9460"
      try:
         self.db.drop_collection_space(self.cs_name)
      except SDBBaseError as e:
         if -34 != e.code:
            self.fail("drop_cs_fail,detail:" + str(e))
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name, {"ReplSize": 0})

   def test_numberlong_9460(self):
      # insert data
      data1 = -9007199254740991
      data2 = 9007199254740991
      data3 = -9007199254740992
      data4 = 9007199254740992
      data5 = 9223372036854775807
      data6 = -9223372036854775808
      record = [{"a": data1}, {"a": data2}, {"a": data3}, {"a": data4}, {"a": data5}, {"a": data6}]
      self.cl.bulk_insert(0, record)
   
      # query data and check
      expect_type = [{"a": "int64"}, {"a": "int64"}, {"a": "int64"}, {"a": "int64"}, {"a": "int64"}, {"a": "int64"}]
      check_Result(self.cl, {}, {"a": {"$type": 2}}, record, expect_type, False)
      
      #query data and check,
   
      # update data
      data1_after_update = 1
      data2_after_update = 0
      data3_after_update = 2
      data4_after_update = 3
      data5_after_update = 4
      data6_after_update = 5
      self.cl.update({"$set": {"a": data1_after_update}}, condition={"a": data1})
      self.cl.update({"$set": {"a": data2_after_update}}, condition={"a": data2})
      self.cl.update({"$set": {"a": data2_after_update}}, condition={"a": data3})
      self.cl.update({"$set": {"a": data2_after_update}}, condition={"a": data4})
      self.cl.update({"$set": {"a": data2_after_update}}, condition={"a": data5})
      self.cl.update({"$set": {"a": data2_after_update}}, condition={"a": data6})
   
      # query data and check
      expect_type_after_update = [{"a": "int32"}, {"a": "int32"}, {"a": "int32"}, {"a": "int32"}, {"a": "int32"}, {"a": "int32"}]
      record_after_update = [{"a": data1_after_update}, {"a": data2_after_update}, {"a": data3_after_update}, {"a": data4_after_update}, {"a": data5_after_update}, {"a": data6_after_update}]
      check_Result(self.cl, {}, {"a": {"$type": 2}}, record_after_update, expect_type_after_update, False)
   
      # update data
      self.cl.update({"$set": {"a": data1}}, condition={"a": data1_after_update})
      self.cl.update({"$set": {"a": data2}}, condition={"a": data2_after_update})
      self.cl.update({"$set": {"a": data3}}, condition={"a": data3_after_update})
      self.cl.update({"$set": {"a": data4}}, condition={"a": data4_after_update})
      self.cl.update({"$set": {"a": data5}}, condition={"a": data5_after_update})
      self.cl.update({"$set": {"a": data6}}, condition={"a": data6_after_update})
   
      # query data and check
      check_Result(self.cl, {}, {"a": {"$type": 2}}, record, expect_type, False)
   
      # delete data
      self.cl.delete()
   
      # query data and check
      check_Result(self.cl, {}, {}, {}, {}, False)
   
      # insert data
      self.cl.bulk_insert(0, record)
   
      # query data and check
      check_Result(self.cl, {}, {"a": {"$type": 2}}, record, expect_type, False)
   
      # delete data
      self.cl.delete(condition={"a": data1})
      self.cl.delete(condition={"a": {"$et": data2}})
      self.cl.delete(condition={"a": {"$et": data3}})
      self.cl.delete(condition={"a": {"$et": data4}})
      self.cl.delete(condition={"a": {"$et": data5}})
      self.cl.delete(condition={"a": {"$et": data6}})
   
      # query data and check
      check_Result(self.cl, {}, {}, {}, {}, False)
      
      #json data insert
      data1 = '{"$numberLong": "-9007199254740991"}'
      data2 = '{"$numberLong": "9007199254740991"}'
      data3 = '{"$numberLong": "-9007199254740992"}'
      data4 = '{"$numberLong": "9007199254740992"}'
      data5 = '{"$numberLong": 9223372036854775807}'
      data6 = '{"$numberLong": "-9223372036854775808"}'
      record = [{"a": loads(data1)}, {"a": loads(data2)}, {"a": loads(data3)}, {"a": loads(data4)}, {"a": loads(data5)}, {"a": loads(data6)}]
      self.cl.bulk_insert(0, record)

      # query data and check
      expect_type = [{"a": "int64"}, {"a": "int64"}, {"a": "int64"}, {"a": "int64"}, {"a": "int64"}, {"a": "int64"}]
      check_Result(self.cl, {}, {"a": {"$type": 2}}, record, expect_type, False)

      # update data
      data1_after_update = 1
      data2_after_update = 0
      data3_after_update = 2
      data4_after_update = 3
      data5_after_update = 4
      data6_after_update = 5
      self.cl.update({"$set": {"a": data1_after_update}}, condition={"a": loads(data1)})
      self.cl.update({"$set": {"a": data2_after_update}}, condition={"a": loads(data2)})
      self.cl.update({"$set": {"a": data2_after_update}}, condition={"a": loads(data3)})
      self.cl.update({"$set": {"a": data2_after_update}}, condition={"a": loads(data4)})
      self.cl.update({"$set": {"a": data2_after_update}}, condition={"a": loads(data5)})
      self.cl.update({"$set": {"a": data2_after_update}}, condition={"a": loads(data6)})

      # query data and check
      expect_type_after_update = [{"a": "int32"}, {"a": "int32"}, {"a": "int32"}, {"a": "int32"}, {"a": "int32"},
                                  {"a": "int32"}]
      record_after_update = [{"a": data1_after_update}, {"a": data2_after_update}, {"a": data3_after_update},
                             {"a": data4_after_update}, {"a": data5_after_update}, {"a": data6_after_update}]
      check_Result(self.cl, {}, {"a": {"$type": 2}}, record_after_update, expect_type_after_update, False)

      # update data
      self.cl.update({"$set": {"a": loads(data1)}}, condition={"a": data1_after_update})
      self.cl.update({"$set": {"a": loads(data2)}}, condition={"a": data2_after_update})
      self.cl.update({"$set": {"a": loads(data3)}}, condition={"a": data3_after_update})
      self.cl.update({"$set": {"a": loads(data4)}}, condition={"a": data4_after_update})
      self.cl.update({"$set": {"a": loads(data5)}}, condition={"a": data5_after_update})
      self.cl.update({"$set": {"a": loads(data6)}}, condition={"a": data6_after_update})

      # query data and check
      check_Result(self.cl, {}, {"a": {"$type": 2}}, record, expect_type, False)

      # delete data
      self.cl.delete()

      # query data and check
      check_Result(self.cl, {}, {}, {}, {}, False)

      # insert data
      self.cl.bulk_insert(0, record)

      # query data and check
      check_Result(self.cl, {}, {"a": {"$type": 2}}, record, expect_type, False)

      # delete data
      self.cl.delete(condition={"a": loads(data1)})
      self.cl.delete(condition={"a": {"$et": loads(data2)}})
      self.cl.delete(condition={"a": {"$et": loads(data3)}})
      self.cl.delete(condition={"a": {"$et": loads(data4)}})
      self.cl.delete(condition={"a": {"$et": loads(data5)}})
      self.cl.delete(condition={"a": {"$et": loads(data6)}})

      # query data and check
      check_Result(self.cl, {}, {}, {}, {}, False)
      
      #json to bson
      json = '{"$numberLong": "9223372036854775807"}'
      self.assertEqual('9223372036854775807', dumps(loads(json)))

      json = '{"$numberLong": "-9223372036854775808"}'
      self.assertEqual('-9223372036854775808', dumps(loads(json)))

      json = '{"$numberLong": "9007199254740992"}'
      self.assertEqual('9007199254740992', dumps(loads(json)))

      json = '{"$numberLong": "-9007199254740992"}'
      self.assertEqual('-9007199254740992', dumps(loads(json)))

      json = '{"$numberLong": "9007199254740991"}'
      self.assertEqual('9007199254740991', dumps(loads(json)))

      json = '{"$numberLong": "-9007199254740991"}'
      self.assertEqual('-9007199254740991', dumps(loads(json)))

      json = '{"$numberLong": "1"}'
      self.assertEqual('1', dumps(loads(json)))

      json = '{"$numberLong": "-1"}'
      self.assertEqual('-1', dumps(loads(json)))
      
      #json to bson,set_js_compatibility(True)
      bson.json_util.set_js_compatibility(True)
      json = '{"$numberLong": "9223372036854775807"}'
      self.assertEqual(json, dumps(loads(json)))

      json = '{"$numberLong": "-9223372036854775808"}'
      self.assertEqual(json, dumps(loads(json)))

      json = '{"$numberLong": "9007199254740992"}'
      self.assertEqual(json, dumps(loads(json)))

      json = '{"$numberLong": "-9007199254740992"}'
      self.assertEqual(json, dumps(loads(json)))
      
      json = '{"$numberLong": "9007199254740991"}'
      self.assertEqual('9007199254740991', dumps(loads(json)))

      json = '{"$numberLong": "-9007199254740991"}'
      self.assertEqual('-9007199254740991', dumps(loads(json)))
      
      json = '{"$numberLong": "1"}'
      self.assertEqual('1', dumps(loads(json)))

      json = '{"$numberLong": "-1"}'
      self.assertEqual('-1', dumps(loads(json)))

      #out of range
      data7 = -9223372036854775809
      record = [{"a": data7}]
      try:
         self.cl.bulk_insert(0, record)
         self.fail("need_an_error" + str(data7))
      except OverflowError as e:
            print(e)
            
      data8 = 9223372036854775808
      record = [{"a": data8}]
      try:
         self.cl.bulk_insert(0, record)
         self.fail("need_an_error" + str(data8))
      except OverflowError as e:
            print(e)

      try:
         self.cl.insert(loads('{"a": {"$numberLong": "9223372036854775808"}}'))
         self.fail("need_an_error" + str({"$numberLong": "9223372036854775808"}))
      except OverflowError as e:
         print(e)

      try:
         self.cl.insert(loads('{"a": {"$numberLong": "-9223372036854775809"}}'))
         self.fail("need_an_error" + str({"$numberLong": "-9223372036854775809"}))
      except OverflowError as e:
         print(e)
         
      #invalid parameter
      try:
         self.cl.insert(loads('{"a": {"$numberLong": "9.9"}}'))
         self.fail("need_an_error" + str({"$numberLong": "9.9"}))
      except ValueError as e:
         print(e)
   
   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)

 
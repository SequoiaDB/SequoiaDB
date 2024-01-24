# @decription: insert binary data
# @testlink:   seqDB-12455
# @interface:  insert(record)
#              update(rule, kwargs)
#              delete(kwargs)
# @author:     zhaoyu 2017-9-7

from pysequoiadb.error import (SDBBaseError)
from dataopeartion.bsoncurd.commlib import *
from lib import testlib
from bson.json_util import loads
from bson.json_util import dumps
import uuid
from bson.binary import Binary
from bson.py3compat import PY3


class TestBinary12455(testlib.SdbTestBase):
   def setUp(self):
      # create cs and cl
      self.cs_name = "cs_12455"
      self.cl_name = "cl_12455"
      try:
         self.db.drop_collection_space(self.cs_name)
      except SDBBaseError as e:
         if (-34 != e.code):
            self.fail("drop_cs_fail:" + str(e))
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)
   
   def test_binary_12455(self):
      
      # insert int data
      data1 = Binary(b"a", 1)
      data2 = Binary(b"b", 2)
      data3 = Binary(b"c", 5)
      data4 = Binary(b"d", 6)
      data5 = Binary(b"e", 128)
      data6 = Binary(b"f", 255)
      data7 = '{"$binary": "aGVsbG8gd29ybGQ=", "$type": "2"}'
      
      record = [{"a": data1}, {"a": data2}, {"a": data3}, {"a": data4}, {"a": data5}, {"a": data6}, {"a": loads(data7)}]
      self.cl.bulk_insert(0, record)
      
      # query data and check
      expect_type = [{"a": "bindata"}, {"a": "bindata"}, {"a": "bindata"}, {"a": "bindata"}, {"a": "bindata"}, {"a": "bindata"}, {"a": "bindata"}]
      check_Result(self.cl, {}, {"a": {"$type": 2}}, record, expect_type, False)
      
      # update data
      data1_after_update = 0
      data2_after_update = 1
      data3_after_update = 2
      data4_after_update = 3
      data5_after_update = 4
      data6_after_update = 5
      data7_after_update = 6
      self.cl.update({"$set": {"a": data1_after_update}}, condition={"a": {"$et": data1}})
      self.cl.update({"$set": {"a": data2_after_update}}, condition={"a": {"$et": data2}})
      self.cl.update({"$set": {"a": data3_after_update}}, condition={"a": {"$et": data3}})
      self.cl.update({"$set": {"a": data4_after_update}}, condition={"a": {"$et": data4}})
      self.cl.update({"$set": {"a": data5_after_update}}, condition={"a": {"$et": data5}})
      self.cl.update({"$set": {"a": data6_after_update}}, condition={"a": {"$et": data6}})
      self.cl.update({"$set": {"a": data7_after_update}}, condition={"a": {"$et": loads(data7)}})
      
      # query data and check
      expect_type_after_update = [{"a": "int32"}, {"a": "int32"}, {"a": "int32"}, {"a": "int32"}, {"a": "int32"}, {"a": "int32"}, {"a": "int32"}]
      record_after_update = [{"a": data1_after_update}, {"a": data2_after_update}, {"a": data3_after_update},
                             {"a": data4_after_update}, {"a": data5_after_update}, {"a": data6_after_update}, {"a": data7_after_update}]
      check_Result(self.cl, {}, {"a": {"$type": 2}}, record_after_update, expect_type_after_update, False)
      
      # update data
      self.cl.update({"$set": {"a": data1}}, condition={"a": data1_after_update})
      self.cl.update({"$set": {"a": data2}}, condition={"a": data2_after_update})
      self.cl.update({"$set": {"a": data3}}, condition={"a": data3_after_update})
      self.cl.update({"$set": {"a": data4}}, condition={"a": data4_after_update})
      self.cl.update({"$set": {"a": data5}}, condition={"a": data5_after_update})
      self.cl.update({"$set": {"a": data6}}, condition={"a": data6_after_update})
      self.cl.update({"$set": {"a": loads(data7)}}, condition={"a": data7_after_update})
      
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
      self.cl.delete(condition={"a": {"$et": data1}})
      self.cl.delete(condition={"a": {"$et": data2}})
      self.cl.delete(condition={"a": {"$et": data3}})
      self.cl.delete(condition={"a": {"$et": data4}})
      self.cl.delete(condition={"a": {"$et": data5}})
      self.cl.delete(condition={"a": {"$et": data6}})
      self.cl.delete(condition={"a": {"$et": loads(data7)}})
      
      # query data and check
      check_Result(self.cl, {}, {}, {}, {}, False)
      
      #uuid
      uuid1= uuid.uuid4()
      uuid2= uuid.uuid4()
      data1 = Binary(uuid1.bytes, 3)
      data2 = Binary(uuid2.bytes, 4)
      record = [{"a": data1}, {"a": data2}]
      self.cl.bulk_insert(0, record)

      expect_type = [{"a": "bindata"}, {"a": "bindata"}]
      expect_record = [{"a": uuid1}, {"a": uuid2}]
      check_Result(self.cl, {}, {"a": {"$type": 2}}, expect_record, expect_type, False)
      self.cl.delete()
      
      #json to bson
      json = data7
      self.assertEqual(json, dumps(loads(json)))
      
      #type set 0
      data = Binary(b"a", 0)
      record = [{"a": data}]
      self.cl.bulk_insert(0, record)

      expect_type = [{"a": "bindata"}]
      expect_record_python3 = [{"a": b'a'}]
      if PY3:
         check_Result(self.cl, {}, {"a": {"$type": 2}}, expect_record_python3, expect_type, False)
      else:
         check_Result(self.cl, {}, {"a": {"$type": 2}}, record, expect_type, False)
      
      #type set 256
      try:
         data = Binary(b"a", 256)
         self.fail("NEED_AN_ERROR")
      except ValueError as e:
         print(e)

      # type set -1
      try:
         data = Binary(b"a", -1)
         self.fail("NEED_AN_ERROR")
      except ValueError as e:
         print(e)
      
   
   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)

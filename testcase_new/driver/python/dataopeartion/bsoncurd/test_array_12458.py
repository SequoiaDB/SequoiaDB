# @decription: insert array data
# @testlink:   seqDB-12458
# @interface:  insert(record)
#              update(rule, kwargs)
#              delete(kwargs)
# @author:     zhaoyu 2017-11-28

from pysequoiadb.error import (SDBBaseError)
from dataopeartion.bsoncurd.commlib import *
from lib import testlib


class TestInt12458(testlib.SdbTestBase):
   def setUp(self):
      # create cs and cl
      self.cs_name = "cs_12458"
      self.cl_name = "cl_12458"
      try:
         self.db.drop_collection_space(self.cs_name)
      except SDBBaseError as e:
         if -34 != e.code:
            self.fail("drop_cs_fail,detail:" + str(e))
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)
   
   def test_int_12448(self):
      # insert int data
      data1 = [-214748364,
               {"$numberLong": "-9223372036854775808"},
               1.7E+308,
               {"$numberLong": "9223372036854775807111"},
               "a",
               {"$oid": "123abcd00ef12358902300ef"},
               True,
               {"$date": "2012-01-01"},
               {"$timestamp": "2012-01-01-13.14.26.124233"},
               {"$binary": "aGVsbG8gd29ybGQ=", "$type": "1"},
               {"$regex": "^a", "$options": "i"},
               {"subobj": "value"},
               ["abc", 0, "def"],
               None,
               {"$minKey": 1},
               {"$maxKey": 1}]
      
      record = [{"a": data1}]
      self.cl.bulk_insert(0, record)

      # query data and check
      expect_type = [{"a": "array"}]
      check_Result(self.cl, {}, {"a": {"$type": 2}}, record, expect_type, False)

      # update data
      data1_after_update = 1
      self.cl.update({"$set": {"a": data1_after_update}}, condition={"a": data1})

      # query data and check
      record_after_update = [{"a": data1_after_update}]
      record_after_update_type = [{"a": "int32"}]
      check_Result(self.cl, {}, {"a": {"$type": 2}}, record_after_update, record_after_update_type, False)

      # update data
      self.cl.update({"$set": {"a": data1}}, condition={"a": data1_after_update})

      # query data and check
      check_Result(self.cl, {}, {"a": {"$type": 2}}, record, expect_type, False)

      # delete data
      self.cl.delete()

      # query data and check
      check_Result(self.cl, {}, {}, {}, {}, False)

      # insert int data
      self.cl.bulk_insert(0, record)

      # query data and check
      check_Result(self.cl, {}, {"a": {"$type": 2}}, record, expect_type, False)

      # delete data
      self.cl.delete(condition={"a": data1})

      # query data and check
      check_Result(self.cl, {}, {}, {}, {}, False)
   
   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)
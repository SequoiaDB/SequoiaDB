# @decription: insert regex data
# @testlink:   seqDB-12456/seqDB-26599
# @interface:  insert(record)
#              update(rule, kwargs)
#              delete(kwargs)
# @author:     zhaoyu 2017-9-6

from pysequoiadb.error import SDBBaseError
from dataopeartion.bsoncurd.commlib import *
from bson.json_util import loads
from bson.json_util import dumps
from bson.regex import Regex
import re
import sys
from lib import testlib
import bson


class TestRegex12456(testlib.SdbTestBase):
   def setUp(self):
      # create cs and cl
      self.cs_name = "cs_12456"
      self.cl_name = "cl_12456"
      try:
         self.db.drop_collection_space(self.cs_name)
      except SDBBaseError as e:
         if -34 != e.code:
            self.fail("drop_cs_fail,detail:" + str(e))
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)
   
   def test_regex_12456(self):
      # insert int data
      data1 = Regex('^a', 'i')
      r1 = Regex('^b', 'i')
      self.assertFalse(data1 == r1)
      if( sys.version_info[:2] <= (3, 5) ):
         data2 = Regex('^b',4)
      else:
         data2 = Regex('^b', re.RegexFlag.DOTALL | re.RegexFlag.UNICODE)
      data3 = Regex.from_native(re.compile(b'.c', 0))
      data4 = '{"$regex": "^d", "$options": "x"}'
      record = [{"a": data1}, {"a": data2}, {"a": data3}, {"a": loads(data4, compile_re=False)}]
      self.cl.bulk_insert(0, record)
      
      
      # query data and check
      if( sys.version_info[0] == 2):
         expect_record =[dumps({'a': data1}),
                         dumps({'a': data2}),
                         dumps({'a': data3}),
                         dumps({'a': loads(data4, compile_re=False)})]
      elif( sys.version_info[:2] <= (3, 5) ):
         expect_record = [dumps({"a": bson.SON([("$regex", "^a"), ("$options", "iu")])}),
                          dumps({"a": bson.SON([("$regex", "^b"), ("$options", "lu")])}),
                          dumps({"a": bson.SON([("$regex", ".c"), ("$options", "u")])}),
                          dumps({"a": bson.SON([("$regex", "^d"), ("$options", "ux")])})]
      else:
         expect_record = [dumps({"a": bson.SON([("$regex", "^a"), ("$options", "iu")])}),
                          dumps({"a": bson.SON([("$regex", "^b"), ("$options", "su")])}),
                          dumps({"a": bson.SON([("$regex", ".c"), ("$options", "u")])}),
                          dumps({"a": bson.SON([("$regex", "^d"), ("$options", "ux")])})]
      expect_type = [{"a": "regex"}, {"a": "regex"}, {"a": "regex"}, {"a": "regex"}]
      self.check_Result(self.cl, {}, {"a": {"$type": 2}}, expect_record, expect_type, False)
      
      # update data
      data1_after_update = 1
      data2_after_update = 0
      data3_after_update = 2
      data4_after_update = 3
      self.cl.update({"$set": {"a": data1_after_update}}, condition={"a": {"$et": data1}})
      self.cl.update({"$set": {"a": data2_after_update}}, condition={"a": {"$et": data2}})
      self.cl.update({"$set": {"a": data3_after_update}}, condition={"a": {"$et": data3}})
      self.cl.update({"$set": {"a": data4_after_update}}, condition={"a": {"$et": loads(data4, compile_re=False)}})
      
      # query data and check
      expect_type_after_update = [{"a": "int32"}, {"a": "int32"}, {"a": "int32"}, {"a": "int32"}]
      record_after_update = [{"a": data1_after_update}, {"a": data2_after_update}, {"a": data3_after_update},
                             {"a": data4_after_update}]
      check_Result(self.cl, {}, {"a": {"$type": 2}}, record_after_update, expect_type_after_update, False)
      
      # update data
      self.cl.update({"$set": {"a": data1}}, condition={"a": data1_after_update})
      self.cl.update({"$set": {"a": data2}}, condition={"a": data2_after_update})
      self.cl.update({"$set": {"a": data3}}, condition={"a": data3_after_update})
      self.cl.update({"$set": {"a": loads(data4)}}, condition={"a": data4_after_update})
      
      # query data and check
      self.check_Result(self.cl, {}, {"a": {"$type": 2}}, expect_record, expect_type, False)
      
      # delete data
      self.cl.delete()
      
      # query data and check
      check_Result(self.cl, {}, {}, {}, {}, False)
      
      # insert data
      self.cl.bulk_insert(0, record)
      
      # query data and check
      self.check_Result(self.cl, {}, {"a": {"$type": 2}}, expect_record, expect_type, False)
      
      # delete data
      self.cl.delete(condition={"a": {"$et": data1}})
      self.cl.delete(condition={"a": {"$et": data2}})
      self.cl.delete(condition={"a": {"$et": data3}})
      self.cl.delete(condition={"a": {"$et": loads(data4, compile_re=False)}})
      
      # query data and check
      check_Result(self.cl, {}, {}, {}, {}, False)
      
      # json to bson
      json = data4
      self.assertEqual(json, dumps(loads(json, compile_re=False)))
      
   #def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)
   
   def check_Result(self, cl, condition, selector, expect_result, expect_type, check_id):
      actual_record_num = cl.get_count(condition=condition)
      expect_record_num = len(expect_result)
      if (actual_record_num != expect_record_num):
         raise Exception('check count error,actual_record_num is:' + str(actual_record_num))
   
      cursor = cl.query(condition=condition)
      while True:
         try:
            record = cursor.next()
            if not check_id:
               del record['_id']
            if dumps(record) not in expect_result:
               raise Exception('check record error, actual record is:' + dumps(record) + ",expect record is :" + str(expect_result))
         except SDBEndOfCursor:
            break
      cursor.close()
   
      expect_type_num = len(expect_type)
      if (actual_record_num != expect_type_num):
         raise Exception('check type count error, actual record is:' + str(actual_record_num))
   
      cursor = cl.query(condition=condition, selector=selector)
      while True:
         try:
            record = cursor.next()
            if not check_id:
               del record['_id']
            if record not in expect_type:
               raise Exception('check type error, actual record is:' + str(record))
      
         except SDBEndOfCursor:
            break
      cursor.close()
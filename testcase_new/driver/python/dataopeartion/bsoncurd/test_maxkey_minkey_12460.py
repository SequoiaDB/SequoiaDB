# @decription: insert minKey/maxKey data
# @testlink:   seqDB-12460
# @interface:  insert(record)
#              update(rule, kwargs)
#              delete(kwargs)
# @author:     zhaoyu 2017-9-6

from pysequoiadb.error import (SDBBaseError)
from dataopeartion.bsoncurd.commlib import *
from lib import testlib
from bson.json_util import loads
from bson.json_util import dumps
from bson.max_key import MaxKey
from bson.min_key import MinKey


class TestMinKeyMaxKey12460(testlib.SdbTestBase):
   def setUp(self):
      # create cs and cl
      self.cs_name = "cs_12460"
      self.cl_name = "cl_12460"
      try:
         self.db.drop_collection_space(self.cs_name)
      except SDBBaseError as e:
         if -34 != e.code:
            self.fail("drop_cs_fail,detail:" + str(e))
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)
   
   def test_minkey_maxkey_12460(self):
      # insert data
      data1 = MaxKey()
      data2 = MinKey()
      data3 = '{"$maxKey": 1}'
      data4 = '{"$minKey": 1}'
      record = [{"a": data1}, {"a": data2}, {"a": loads(data3)}, {"a": loads(data4)}]
      self.cl.bulk_insert(0, record)
      
      # query data and check
      expect_type = [{"a": "maxkey"}, {"a": "minkey"}, {"a": "maxkey"}, {"a": "minkey"}]
      check_Result(self.cl, {}, {"a": {"$type": 2}}, record, expect_type, False)
      
      # update data
      data1_after_update = 1
      data2_after_update = 0
      self.cl.update({"$set": {"a": data1_after_update}}, condition={"a": data1})
      self.cl.update({"$set": {"a": data2_after_update}}, condition={"a": data2})
      
      # query data and check
      expect_type_after_update = [{"a": "int32"}, {"a": "int32"}, {"a": "int32"}, {"a": "int32"}]
      record_after_update = [{"a": data1_after_update}, {"a": data2_after_update}, {"a": data1_after_update}, {"a": data2_after_update}]
      check_Result(self.cl, {}, {"a": {"$type": 2}}, record_after_update, expect_type_after_update, False)
      
      # update data
      self.cl.update({"$set": {"a": data2}}, condition={"a": data2_after_update})
      self.cl.update({"$set": {"a": data1}}, condition={"a": data1_after_update})
      
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
      
      # query data and check
      check_Result(self.cl, {}, {}, {}, {}, False)
      
      # json to bson
      json = '{"a": {"$minKey": 1}}'
      self.assertEqual(json, dumps(loads(json)))

      json = '{"a": {"$maxKey": 1}}'
      self.assertEqual(json, dumps(loads(json)))
      
   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)
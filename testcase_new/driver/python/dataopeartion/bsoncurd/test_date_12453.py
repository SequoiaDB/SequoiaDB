# @decription: insert date data
# @testlink:   seqDB-12453
# @interface:  insert(record)
#              update(rule, kwargs)
#              delete(kwargs)
# @author:     zhaoyu 2017-9-1

import datetime
from pysequoiadb.error import (SDBBaseError)
from dataopeartion.bsoncurd.commlib import *
from lib import testlib
from bson.json_util import loads 
from bson.json_util import dumps

class TestDate12453(testlib.SdbTestBase):
   def setUp(self):
      # create cs and cl
      self.cs_name = "cs_12453"
      self.cl_name = "cl_12453"
      try:
         self.db.drop_collection_space(self.cs_name)
      except SDBBaseError as e:
         if -34 != e.code:
            self.fail("drop_cs_fail,detail:" + str(e))
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)

   def test_date_12453(self):
      #insert data
      data1 = '{"$date":"0002-01-01"}'
      data2 = '{"$date":"9998-12-31"}'
      data3 = datetime.datetime(1, 1, 1)
      data4 = datetime.datetime(9999, 12, 31)
      record = [{"a": loads(data1)}, {"a": loads(data2)}, {"a": data3}, {"a": data4}]
      self.cl.bulk_insert(0, record)
      
      #query data and check
      expect_type = [{"a": "date"}, {"a": "date"}, {"a": "date"}, {"a": "date"}]
      check_Result(self.cl, {}, {"a": {"$type": 2}}, record, expect_type, False)
      
      #update data
      data1_after_update = 1
      data2_after_update = 0
      data3_after_update = -1
      data4_after_update = -2
      self.cl.update({"$set": {"a": data1_after_update}}, condition={"a": loads(data1)})
      self.cl.update({"$set": {"a": data2_after_update}}, condition={"a": loads(data2)})
      self.cl.update({"$set": {"a": data3_after_update}}, condition={"a": data3})
      self.cl.update({"$set": {"a": data4_after_update}}, condition={"a": data4})
      
      #query data and check
      expect_type_after_update = [{"a": "int32"}, {"a": "int32"}, {"a": "int32"}, {"a": "int32"}]
      record_after_update = [{"a": data1_after_update},{"a": data2_after_update}, {"a": data3_after_update}, {"a": data4_after_update}]
      check_Result(self.cl, {}, {"a": {"$type": 2}}, record_after_update, expect_type_after_update, False)
      
      #update data
      self.cl.update({"$set": {"a": loads(data1)}}, condition={"a": data1_after_update})
      self.cl.update({"$set": {"a": loads(data2)}}, condition={"a": data2_after_update})
      self.cl.update({"$set": {"a": data3}}, condition={"a": data3_after_update})
      self.cl.update({"$set": {"a": data4}}, condition={"a": data4_after_update})
      
      #query data and check
      check_Result(self.cl, {}, {"a": {"$type": 2}}, record, expect_type, False)
      
      #delete data
      self.cl.delete()
      
      #query data and check
      check_Result(self.cl, {}, {}, {}, {}, False)
      
      #insert data 
      self.cl.bulk_insert( 0, record )
      
      #query data and check
      check_Result( self.cl, {}, {"a":{"$type":2}}, record, expect_type, False )
      
      #delete data
      self.cl.delete(condition={"a": loads(data1)})
      self.cl.delete(condition={"a": {"$et": loads(data2)}})
      self.cl.delete(condition={"a": {"$et": data3}})
      self.cl.delete(condition={"a": {"$et": data4}})
      
      #query data and check
      check_Result(self.cl, {}, {}, {}, {}, False)
      
      #json to bson   
      json = '{"$date": "0001-01-01"}'
      self.assertEqual(json, dumps(loads(json)))

      json = '{"$date": "9999-12-31"}'
      self.assertEqual(json, dumps(loads(json)))

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)
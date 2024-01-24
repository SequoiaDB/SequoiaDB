# @decription: create cl set compress and check options
# @testlink:   seqDB-12446
# @interface:  create_collection(cl_name)
#              create_collection(cl_name,options):options set dict,
#                 keys set AutoIndexId/EnsureShardingIndex/Compressed/CompressionType:"snappy"/"lzw"
#              drop_collection(cl_name)
#              list_collections()
# @author:     zhaoyu 2017-8-29

from pysequoiadb.error import (SDBBaseError, SDBEndOfCursor)
from lib import testlib

class TestMeteData12446(testlib.SdbTestBase):
   def setUp(self):
      if testlib.is_standalone():
         self.skipTest("run mode is standalone")
      if testlib.get_data_group_num() == 1:
         self.skipTest("run mode is one group")
      self.cs_name = "cs_12446"
      
   def test_metedata_12446(self):
      #create cs
      cl_names = ["cl_12446_1", "cl_12446_2"]
      try:
         self.db.drop_collection_space(self.cs_name)
      except SDBBaseError as e:
         if -34 != e.code:
            self.fail("drop_cs_fail,detail:" + str(e))
      self.cs = self.db.create_collection_space(self.cs_name)
      
      #create cl set Compressed
      cl_options_1 = {"ShardingKey": {"a": 1}, "ShardingType": "range",
                      "AutoIndexId": False, "EnsureShardingIndex": False,
                      "Compressed": True, "CompressionType": "lzw"}
      self.cs.create_collection(cl_names[0], cl_options_1)
      
      cl_options_2 = {"ShardingKey": {"a": 1}, "ShardingType": "range",
                      "AutoIndexId": False, "EnsureShardingIndex": False,
                      "Compressed": True}
      self.cs.create_collection(cl_names[1], cl_options_2)
      
      #check cl
      except_cl_options_1 = {"Attribute": 3, "AttributeDesc": "Compressed | NoIDIndex",
                             "CompressionType": 1, "CompressionTypeDesc": "lzw",
                             "ShardingKey": {"a": 1}, "EnsureShardingIndex": False,
                             "ShardingType": "range"}
      self.check_cl_snapshot_8(self.cs_name + "." + cl_names[0], except_cl_options_1)
      
      except_cl_options_2 = {"Attribute": 3, "AttributeDesc": "Compressed | NoIDIndex",
                             "CompressionType": 1, "CompressionTypeDesc": "lzw",
                             "ShardingKey": {"a": 1}, "EnsureShardingIndex": False,
                             "ShardingType": "range"}
      self.check_cl_snapshot_8(self.cs_name + "." + cl_names[1], except_cl_options_2)
      
   def tearDown(self):
      if self.should_clean_env():
         try:
            self.db.drop_collection_space(self.cs_name)
         except SDBBaseError as e:
            if -34 != e.code:
               self.fail("tear_down_fail,detail:" + str(e))
            
   def check_cl_snapshot_8(self, cl_full_name, options):
      cursor = self.db.get_snapshot(8, condition={"Name": cl_full_name})
      while True:
         try:
            record = cursor.next()
            self.assertDictContainsSubset(options, record)
         except SDBEndOfCursor:
            break
      cursor.close()
# @decription: create maincl and subcl and check options
# @testlink:   seqDB-12445
# @interface:  create_collection(cl_name)
#              create_collection(cl_name,options):options set dict,
#                 keys set IsMainCL/Group/ReplSize/ShardingKey/ShardingType/Partition
#              drop_collection(cl_name)
#              list_collections()
#              attach_collection(cl_full_name,options),options set LowBound/UpBound
#              detach_collection(sub_cl_full_name)
#              list_collection_spaces()
#              drop_collection_space(cs_name)
# @author:     zhaoyu 2017-8-29

from lib import testlib
from pysequoiadb.error import (SDBBaseError, SDBEndOfCursor)

class TestMeteData12445(testlib.SdbTestBase):
   def setUp(self):
      if testlib.is_standalone():
         self.skipTest("run mode is standalone")
      if testlib.get_data_group_num() == 1:
         self.skipTest("run mode is one group")
      self.maincs_name = "maincs_12445"
      self.subcs_name = "subcs_12445"
      self.maincl_name = "maincl_12445"
      self.subcl_names = ["subcl_12445_1", "subcl_12445_2", "subcl_12445_3"]
      self.maincl_full_name = self.maincs_name + "." + self.maincl_name
      self.subcl_full_name1 = self.subcs_name + "." + self.subcl_names[0]
      self.subcl_full_name2 = self.subcs_name + "." + self.subcl_names[1]
      self.subcl_full_name3 = self.subcs_name + "." + self.subcl_names[2]
      
   def test_metedata_12445(self):
      data_groups = testlib.get_data_groups()
      #get data_groups
      self.cl_group_name =  data_groups[0]['GroupName']
      
      #create cs
      try:
         self.db.drop_collection_space(self.maincs_name)
      except SDBBaseError as e:
         if -34 != e.code:
            self.fail("drop_cs_fail,detail:" + str(e))
      try:
         self.db.drop_collection_space(self.subcs_name)
      except SDBBaseError as e:
         if -34 != e.code:
            self.fail("drop_cs_fail,detail:" + str(e))
      self.maincs = self.db.create_collection_space(self.maincs_name)
      self.subcs = self.db.create_collection_space(self.subcs_name)
      
      #create maincl
      maincl_options = {"IsMainCL":True,"ShardingKey":{"a":1},"ShardingType":"range"}
      self.maincl = self.maincs.create_collection(self.maincl_name,maincl_options)
      
      #create subcl
      self.subcs.create_collection(self.subcl_names[0])
      subcl_options2 = {"ShardingKey": {"b": 1}, "ShardingType": "range", "Group": self.cl_group_name, "ReplSize": -1}
      self.subcs.create_collection(self.subcl_names[1], subcl_options2)
      subcl_options3 = {"ShardingKey": {"_id": 1}, "ShardingType": "hash", "Group": self.cl_group_name, "Partition": 4096}
      self.subcs.create_collection(self.subcl_names[2], subcl_options3)
      
      #list cl
      cursor = self.db.list_collections()
      expect_cl_names = [self.maincl_full_name,
                        self.subcl_full_name1,
                        self.subcl_full_name2,
                        self.subcl_full_name3]
      actual_cl_names = []
      while True:
         try:
            record = cursor.next()
            cl_name = record['Name']
            for index in range(len(expect_cl_names)):
               if(cl_name == expect_cl_names[index]):
                  actual_cl_names.append(cl_name)          
         except SDBEndOfCursor:
            break
      cursor.close()
      self.assertListEqualUnordered(expect_cl_names, actual_cl_names)
      
      #attach cl
      attach_option1 = {"LowBound": {"a": 1}, "UpBound": {"a": 1000}}
      self.maincl.attach_collection(self.subcl_full_name1, attach_option1)
      
      attach_option2 = {"LowBound": {"a": 1000}, "UpBound": {"a": 2000}}
      self.maincl.attach_collection(self.subcl_full_name2, attach_option2)
      
      attach_option3 = {"LowBound": {"a": 2000}, "UpBound": {"a": 3000}}
      self.maincl.attach_collection(self.subcl_full_name3, attach_option3)
      
      #check sub cl options
      except_subcl_options_1 = {"Attribute": 1, "AttributeDesc": "Compressed",
                                "MainCLName": self.maincl_full_name, "Name": self.subcl_full_name1}
      self.check_cl_snapshot_8(self.subcl_full_name1, except_subcl_options_1)
      
      except_subcl_options_2 = {"Attribute": 1, "AttributeDesc": "Compressed", "CataInfo": 1,
                                "MainCLName": self.maincl_full_name, "Name": self.subcl_full_name2,
                                "EnsureShardingIndex": True, "ShardingKey": {"b":1}, "ShardingType": "range",
                                "ReplSize": -1}
      self.check_cl_snapshot_8(self.subcl_full_name2, except_subcl_options_2)
      
      except_subcl_options_3 = {"Attribute": 1, "AttributeDesc": "Compressed", "CataInfo": 1,
                                "MainCLName": self.maincl_full_name, "Name": self.subcl_full_name3,
                                "EnsureShardingIndex": True, "ShardingKey": {"_id": 1}, "ShardingType": "hash",
                                "Partition": 4096}
      self.check_cl_snapshot_8(self.subcl_full_name3, except_subcl_options_3)
      
      #check main cl options
      except_maincl_options = {"Attribute": 1, "AttributeDesc": "Compressed",
                               "IsMainCL": True, "Name": self.maincl_full_name,
                               "EnsureShardingIndex": True, "ShardingKey": {"a": 1}, "ShardingType": "range"}
      self.check_cl_snapshot_8(self.maincl_full_name, except_maincl_options)
      
      #detach cl
      self.maincl.detach_collection(self.subcl_full_name1)
      self.maincl.detach_collection(self.subcl_full_name2)
      self.maincl.detach_collection(self.subcl_full_name3)
      
      #check subcl
      self.check_subcl_after_detach(self.subcl_full_name1)
      self.check_subcl_after_detach(self.subcl_full_name2)
      self.check_subcl_after_detach(self.subcl_full_name3)
      
      #check maincl
      self.check_maincl_after_detach(self.maincl_full_name)
      
      #drop cl
      self.subcs.drop_collection(self.subcl_names[0])
      self.subcs.drop_collection(self.subcl_names[1])
      self.subcs.drop_collection(self.subcl_names[2])
      
      self.maincs.drop_collection(self.maincl_name)
      
      #list cl
      cursor = self.db.list_collections()
      expect_cl_names = [self.maincl_full_name,
                        self.subcl_full_name1,
                        self.subcl_full_name2,
                        self.subcl_full_name3]
      while True:
         try:
            record = cursor.next()
            cl_name = record['Name']
            for index in range(len(expect_cl_names)):
               if(cl_name == expect_cl_names[index]):
                  print("cl_name:%s" %cl_name)
                  self.fail("cl_not_drop")           
         except SDBEndOfCursor:
            break
      cursor.close()

      #dropCS
      self.db.drop_collection_space(self.subcs_name)
      self.db.drop_collection_space(self.maincs_name)
   
   def tearDown(self):
      if self.should_clean_env():
         try:
            self.db.drop_collection_space(self.subcs_name)
            self.db.drop_collection_space(self.maincs_name)
         except SDBBaseError as e:
            if -34 != e.code:
               self.fail("tear_down_fail,detail:" + str(e))
   
   def check_cl_snapshot_8(self, cl_full_name, options):
      cursor = self.db.get_snapshot(8, condition={"Name": cl_full_name})
      while True:
         try:
            record = cursor.next()
            actual_keys = record.keys()
            expect_keys = options.keys()
            for expect_key in expect_keys:
               has_key = False
               for atual_key in actual_keys:
                  if expect_key == atual_key:
                     if expect_key == "CataInfo":
                        self.assertEqual(record[expect_key][0]["GroupName"], self.cl_group_name)
                     else:
                        self.assertEqual(record[atual_key], options[expect_key])
                     has_key = True
               self.assertTrue(has_key)
         except SDBEndOfCursor:
            break
      cursor.close()
      
   def check_subcl_after_detach(self, subcl_full_name):
      cursor = self.db.get_snapshot(8, condition={"Name": subcl_full_name})
      while True:
         try:
            record = cursor.next()
            self.assertFalse('MainCLName' in record)
         except SDBEndOfCursor:
            break
      cursor.close()
   
   def check_maincl_after_detach(self, maincl_full_name):
      cursor = self.db.get_snapshot(8, condition={"Name":maincl_full_name})
      while True:
         try:
            record = cursor.next()
            atual_cataInfo = record['CataInfo']
            self.assertEqual(len(atual_cataInfo), 0)
         except SDBEndOfCursor:
            break
      cursor.close()
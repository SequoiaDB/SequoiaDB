# @decription: test list storageunits
# @testlink:   seqDB-12504
# @interface:  get_list(self,list_type,kwargs)
# @author:     liuxiaoxuan 2017-02-22

import unittest
import datetime
from pysequoiadb.error import SDBBaseError
from lib import testlib

sdb_list_storageunits = 6
class TestListStorageUnits12504(testlib.SdbTestBase):
   def setUp(self):
      # skip standalone
      if testlib.is_standalone():
         self.skipTest('run mode is standalone')
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)
      
   def test_list_storageunits_12504(self):
      # create cl
      groups = testlib.get_data_groups()
      group_name = groups[0]['GroupName']
      self.cl = self.cs.create_collection(self.cl_name)
      
      # get data db
      data_db = self.get_datadb(group_name)
   
      expect_result = ["ID", "CollectionHWM"]
      # list 6
      cursor = data_db.get_list(sdb_list_storageunits)
      act_result = testlib.get_all_records(cursor)
      # check list
      self.check_list(expect_result, act_result)
      
      data_db.disconnect()
		
   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)

   def get_datadb(self, group_name):  
      datadb = None   
      try:
         rg = self.db.get_replica_group_by_name(group_name)
         datadb = rg.get_master().connect()
      except BaseException as e:
         self.fail('get datadb fail: ' + str(e))
      return datadb   
         
   def check_list(self, expect_result, act_result):
      is_has_id = False
      is_has_collection_hwm = False
      for x in act_result:
         if expect_result[0] in x:
            is_has_id = True
         if expect_result[1] in x:
            is_has_collection_hwm = True
         self.assertTrue(is_has_id)
         self.assertTrue(is_has_collection_hwm)

# @decription: test enable capped, disable capped
# @testlink:   seqDB-15231
# @interface:  enable_capped(self)
# @interface:  disable_capped(self)
# @author:     liuxiaoxuan 2018-04-27

import unittest
from bson.py3compat import (long_type)
from pysequoiadb.error import SDBBaseError
from alter.commlib import *
from lib import testlib

class TestEnableDisableCapped15231(testlib.SdbTestBase):
   def setUp(self):
      # skip standalone
      if testlib.is_standalone():
         self.skipTest('run mode is standalone')
      # create cs      
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)

   def test_enable_disable_capped_15231(self):
      
      # check before enable cs
      expect_cs_attributes = [{'Type': 0}]
      self.check_collection_space_attributes(expect_cs_attributes, condition = {'Name' : self.cs_name})
      
      # enable cs capped
      self.cs.enable_capped()
      
      # create cl capped
      cl_name = 'test15231_cappedcl'
      self.cs.create_collection(cl_name, {'Capped': True, 'Size':1, 'Max': 10000, 'AutoIndexId': False})
      
      # check cs attributes after enable 
      expect_cs_attributes = [{'Type': 1}]
      self.check_collection_space_attributes(expect_cs_attributes, condition = {'Name' : self.cs_name})
      
      # check cl attributes after enable (AutoIndexId cannot show on snapshot(8))
      expect_cl_attributes = [{'AttributeDesc': 'NoIDIndex | Capped', 'Max': long_type(10000), 'Size': long_type(33554432)}]
      self.check_collection_attributes(expect_cl_attributes, condition = {'Name' : self.cs_name + '.' + cl_name})
      
      # disable capped, must fail
      try:
         self.cs.disable_capped() 
         self.fail('disable capped success!')       
      except SDBBaseError as e:
         self.assertEqual(e.code, -275)  
      
      # check cs after disable fail
      expect_cs_attributes = [{'Type': 1}]
      self.check_collection_space_attributes(expect_cs_attributes, condition = {'Name' : self.cs_name})
      
      # drop cl
      self.cs.drop_collection(cl_name)
      
      # disable capped success
      self.cs.disable_capped() 
      
      # check cs after disable success
      expect_cs_attributes = [{'Type': 0}]
      self.check_collection_space_attributes(expect_cs_attributes, condition = {'Name' : self.cs_name})
      
      # create new common cl
      cl_name = 'test15231_commoncl'
      self.cs.create_collection(cl_name)
      
      # enable cs when exists cl, must fail
      try:
         self.cs.enable_capped() 
         self.fail('enable capped success!')       
      except SDBBaseError as e:
         self.assertEqual(e.code, -275)  
         
      # check cs after enable fail
      expect_cs_attributes = [{'Type': 0}]
      self.check_collection_space_attributes(expect_cs_attributes, condition = {'Name' : self.cs_name})

   def tearDown(self):
      self.db.drop_collection_space(self.cs_name)
      self.db.disconnect()
      
   def get_collection_space_attrbutes(self, **kwargs):
      # get system collection spaces
      cata_host = self.db.get_replica_group_by_name("SYSCatalogGroup").get_master().get_hostname()
      cata_svc = self.db.get_replica_group_by_name("SYSCatalogGroup").get_master().get_servicename()
      cata_db = client(cata_host, cata_svc)
      sys_collection_spaces = cata_db.get_collection('SYSCAT.SYSCOLLECTIONSPACES')
   
      # get origin attributes
      act_cs_attri = list()
      cursor = sys_collection_spaces.query(**kwargs)
      while(True):
         try:
            act_cs_attri.append(cursor.next())
         except SDBEndOfCursor:     		
            break
   
      # only check item 'Type'  
      need_check_keys = ['Type']
      for x in act_cs_attri:
         for key in x.copy():
            if key not in need_check_keys:
               x.pop(key)            
      return act_cs_attri   
      
   def check_collection_space_attributes(self, expect_cs_attributes, **kwargs):
      act_cs_attributes = self.get_collection_space_attrbutes(**kwargs)
      act_cs_attributes = get_sort_result(act_cs_attributes)
      expect_cs_attributes = get_sort_result(expect_cs_attributes)
      self.assertEqual(act_cs_attributes, expect_cs_attributes)   
      
   def check_collection_attributes(self, expect_cl_attributes, **kwargs):
      act_cl_attributes = get_collection_attributes(self.db, **kwargs)
      act_cl_attributes = get_sort_result(act_cl_attributes)
      expect_cl_attributes = get_sort_result(expect_cl_attributes)
      self.assertEqual(act_cl_attributes, expect_cl_attributes)
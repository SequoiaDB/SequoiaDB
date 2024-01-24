# @decription: test cs set attributes
# @testlink:   seqDB-15237
# @interface:  set_attributes(self, options)
# @author:     liuxiaoxuan 2018-04-27

import unittest
from pysequoiadb.error import (SDBBaseError, SDBEndOfCursor)
from alter.commlib import *
from lib import testlib

class TestCSSetAttributes15237(testlib.SdbTestBase):
   def setUp(self):
      # skip standalone
      if testlib.is_standalone():
         self.skipTest('run mode is standalone')
      
      # create cs       
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)

   def test_cs_set_attributes_15237(self):
      
      domain_name1 = 'testsetattributescs15237_1'
      domain_name2 = 'testsetattributescs15237_2'
      
      # drop domain if exists
      try:
         self.db.drop_domain(domain_name1)
      except SDBBaseError as e:     		
         self.assertEqual(e.code, -214)   
      try:
         self.db.drop_domain(domain_name2)
      except SDBBaseError as e:     		
         self.assertEqual(e.code, -214)
         
       # get groups
      groups = testlib.get_data_groups()
      group_size = len(groups)
      domain_group_names1 = [groups[i]['GroupName'] for i in range(group_size - 1)]
      domain_group_names2 = [groups[i]['GroupName'] for i in range(group_size)]
      
      # create domains
      domain1 = self.db.create_domain(domain_name1, options =  { 'Groups': domain_group_names1 })
      domain2 = self.db.create_domain(domain_name2, options =  { 'Groups': domain_group_names2 })
      
      # alter when cs not exist domain
      alter_opts1 = {'Domain': domain_name1, 'PageSize': 4096, 'LobPageSize': 8192}
      self.cs.set_attributes(options = alter_opts1)
      
      # check result
      expect_attributes1 = [{'Domain': domain_name1, 'PageSize': 4096, 'LobPageSize': 8192}];
      self.check_collection_space_attrbutes(expect_attributes1, condition = {'Name' : self.cs_name})
      
      # alter when exist domain
      alter_opts1 = {'Domain': domain_name2, 'PageSize': 16384, 'LobPageSize': 32768}
      self.cs.set_attributes(options = alter_opts1)
      
      # check result
      expect_attributes1 = [{'Domain': domain_name2, 'PageSize': 16384, 'LobPageSize': 32768}];
      self.check_collection_space_attrbutes(expect_attributes1, condition = {'Name' : self.cs_name})
        
   def tearDown(self):
      # drop cs
      self.db.drop_collection_space(self.cs_name)
      # remove domain
      self.db.drop_domain('testsetattributescs15237_1')
      self.db.drop_domain('testsetattributescs15237_2')
      self.db.disconnect()
      
   def check_collection_space_attrbutes(self, expect_cs_attri, **kwargs):
      act_cs_attri = get_collection_space_attrbutes(self.db, **kwargs)
      act_cs_attri = get_sort_result(act_cs_attri)
      expect_cs_attri = get_sort_result(expect_cs_attri)
      self.assertEqual(act_cs_attri, expect_cs_attri)
       
# @decription: test alter cs domains
# @testlink:   seqDB-15213
# @interface:  set_domain(self,options)
# @interface:  remove_domain(self)
# @author:     liuxiaoxuan 2018-04-25

import unittest
from pysequoiadb.error import (SDBBaseError, SDBEndOfCursor)
from alter.commlib import *
from lib import testlib

class TestAlterCSDomain15213(testlib.SdbTestBase):
   def setUp(self):
      # skip standalone
      if testlib.is_standalone():
         self.skipTest('run mode is standalone')
      # skip one group   
      if 2 > testlib.get_data_group_num():
         self.skipTest('less than two groups')   
         
      self.domain_name1 = 'testdomain15213_1'
      self.domain_name2 = 'testdomain15213_2'
         
   def test_alter_cs_domain_15213(self):
    
      # drop domain if exists
      try:
         self.db.drop_domain(self.domain_name1)
      except SDBBaseError as e:     		
         self.assertEqual(e.code, -214)
      try:
         self.db.drop_domain(self.domain_name2)
      except SDBBaseError as e:     		
         self.assertEqual(e.code, -214)   
    
      # get groups
      groups = testlib.get_data_groups()
      group_size = len(groups)
      domain_group_names1 = [groups[i]['GroupName'] for i in range(group_size - 1)]
      domain_group_names2 = [groups[i]['GroupName'] for i in range(group_size)]
      
      # create domains
      domain1 = self.db.create_domain(self.domain_name1, options =  { 'Groups': domain_group_names1 })
      domain2 = self.db.create_domain(self.domain_name2, options =  { 'Groups': domain_group_names2 })
      
      # create cs cl
      cs_name = 'testaltercsdomain15213_cs'
      cl_name = 'testaltercsdomain15213_cl'
      cs = self.db.create_collection_space(cs_name)
      cs.create_collection(cl_name, {'Group': domain_group_names1[0]})
      
      # check cs before add domain
      expect_cs_attri = [{'PageSize': 65536, 'LobPageSize': 262144}]
      self.check_collection_space_attrbutes(expect_cs_attri, condition = {'Name' : cs_name})
      
      # add domain
      cs.set_domain(options = {'Domain' : self.domain_name1})
      
      # check cs after add domain
      expect_cs_attri = [{'Domain': self.domain_name1, 'PageSize': 65536, 'LobPageSize': 262144}]
      self.check_collection_space_attrbutes(expect_cs_attri, condition = {'Name' : cs_name})

      # set domain
      cs.set_domain(options = {'Domain' : self.domain_name2})
      
      # check cs after alter domain
      expect_cs_attri = [{'Domain': self.domain_name2, 'PageSize': 65536, 'LobPageSize': 262144}]
      self.check_collection_space_attrbutes(expect_cs_attri, condition = {'Name' : cs_name})
      
      # set domain, remove domain
      cs.remove_domain()
      
      # check cs after remove domain
      expect_cs_attri = [{'PageSize': 65536, 'LobPageSize': 262144}]
      self.check_collection_space_attrbutes(expect_cs_attri, condition = {'Name' : cs_name})
  
      # drop cs
      self.db.drop_collection_space(cs_name)
		
   def tearDown(self):
      try:
         self.db.drop_collection_space('testaltercsdomain15213_cs')
      except SDBBaseError as e:     		
         self.assertEqual(e.code, -34)
      self.db.drop_domain(self.domain_name1)
      self.db.drop_domain(self.domain_name2)
      self.db.disconnect()
      
   def check_collection_space_attrbutes(self, expect_cs_attri, **kwargs):
      act_cs_attri = get_collection_space_attrbutes(self.db, **kwargs)
      act_cs_attri = get_sort_result(act_cs_attri)
      expect_cs_attri = get_sort_result(expect_cs_attri)
      self.assertEqual(act_cs_attri, expect_cs_attri)
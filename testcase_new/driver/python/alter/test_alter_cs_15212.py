# @decription: test alter cs
# @testlink:   seqDB-15212
# @interface:  alter(self,options)
# @author:     liuxiaoxuan 2018-04-25

import unittest
from pysequoiadb.error import SDBBaseError
from alter.commlib import *
from lib import testlib

class TestAlterCS15212(testlib.SdbTestBase):
   def setUp(self):
      # skip standalone
      if testlib.is_standalone():
         self.skipTest('run mode is standalone')
      
      self.domain_name1 = 'testdomain15212_1'
      self.domain_name2 = 'testdomain15212_2'
         
   def test_alter_cs_15212(self):
    
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
      group_names = [x['GroupName'] for x in groups]
      
      # create domains
      domain1 = self.db.create_domain(self.domain_name1, options =  { 'Groups': group_names })
      domain2 = self.db.create_domain(self.domain_name2, options =  { 'Groups': group_names })
      
      # create cs 
      cs_name = 'testaltercs15212'
      cs = self.db.create_collection_space(cs_name, {'Domain': self.domain_name1})
      
      # check before alter
      expect_cs_attri = [{'Domain': self.domain_name1, 'PageSize': 65536, 'LobPageSize': 262144}]
      self.check_collection_space_attrbutes(expect_cs_attri, condition = {'Name' : cs_name})
      
      # alter cs
      cs.alter(options = {'Domain': self.domain_name2, 'PageSize': 4096, 'LobPageSize': 8192})
      
      # check after alter
      expect_cs_attri = [{'Domain': self.domain_name2, 'PageSize': 4096, 'LobPageSize': 8192}]
      self.check_collection_space_attrbutes(expect_cs_attri, condition = {'Name' : cs_name})
      
      # bulk alter, ignore exception
      bulk_opts = {'Alter':[ {'Name': 'set attributes', 'Args': {'Domain':self.domain_name1}}, 
                             {'Name': 'set attributes', 'Args': {'Name':'cs'}}, 
                             {'Name': 'set attributes', 'Args': {'PageSize': 16384}},                             
                             {'Name': 'set attributes', 'Args': {'LobPageSize': 131072}}], 
                   'Options': {'IgnoreException': True}}                           
      cs.alter(options = bulk_opts)
      
      # check after bulk alter
      expect_cs_attri = [{'Domain': self.domain_name1, 'PageSize': 16384, 'LobPageSize': 131072}]
      self.check_collection_space_attrbutes(expect_cs_attri, condition = {'Name' : cs_name})
      
      # bulk alter, not ignore exception, must fail
      bulk_opts = {'Alter':[ {'Name': 'set attributes', 'Args': {'PageSize': 65536}},
                             {'Name': 'set attributes', 'Args': {'Name':'cs'}},                          
                             {'Name': 'set attributes', 'Args': {'LobPageSize': 524288}}], 
                   'Options': {'IgnoreException': False}}              
      try:
         cs.alter(options = bulk_opts)
         self.fail('need alter fail')
      except SDBBaseError as e:     		
         self.assertEqual(e.code, -32)     

      # check after alter failed
      expect_cs_attri = [{'Domain': self.domain_name1, 'PageSize': 65536, 'LobPageSize': 131072}]
      self.check_collection_space_attrbutes(expect_cs_attri, condition = {'Name' : cs_name})         
      
      # drop cs
      self.db.drop_collection_space(cs_name)
		
   def tearDown(self):
      try:
         self.db.drop_collection_space('testaltercs15212')
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
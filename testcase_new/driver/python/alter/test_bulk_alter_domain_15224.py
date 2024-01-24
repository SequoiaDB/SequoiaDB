# @decription: test alter domain
# @testlink:   seqDB-15224
# @interface:  alter(self,options)
# @author:     liuxiaoxuan 2018-04-26

import unittest
from pysequoiadb.error import (SDBBaseError, SDBEndOfCursor)
from alter.commlib import *
from lib import testlib

class TestAlterDomain15224(testlib.SdbTestBase):
   def setUp(self):
      # skip standalone
      if testlib.is_standalone():
         self.skipTest('run mode is standalone')
      self.domain_name = 'testdomain15224'
         
   def test_alter_domain_15224(self):
    
      # drop domain if exists
      try:
         self.db.drop_domain(self.domain_name)
      except SDBBaseError as e:     		
         self.assertEqual(e.code, -214)
    
      # get groups
      groups = testlib.get_data_groups()
      group_names = [x['GroupName'] for x in groups]
      
      # create domains
      domain = self.db.create_domain(self.domain_name, options = { 'AutoSplit': False, 'Groups': group_names })
      
      # check before bulk alter
      expect_bulk_attributes = [{'AutoSplit': False, 'Groups': group_names}]
      self.check_domain_cs(expect_bulk_attributes, condition = {'Name' : self.domain_name})
      
      # bulk alter attributes, ignore exception
      if 1 < len(group_names):
         del group_names[0]
      bulk_opts = {'Alter': [ {'Name': 'set attributes', 'Args': { 'AutoSplit' : True}}, 
                              {'Name': 'set attributes', 'Args': { 'Name': 'newdomain'}}, 
                              {'Name': 'set attributes', 'Args': {'Groups' : group_names}}],
                   'Options': {'IgnoreException':True}}
      domain.alter(options = bulk_opts)
  
      # check after bulk alter
      expect_bulk_attributes = [{'AutoSplit': True, 'Groups': group_names}]
      self.check_domain_cs(expect_bulk_attributes, condition = {'Name' : self.domain_name})

      # bulk alter, not ignore exception, must fail
      bulk_opts = {'Alter': [ {'Name': 'set attributes', 'Args': { 'Name': 'newdomain'}}, 
                              {'Name': 'set attributes', 'Args': { 'AutoSplit' : False}}, 
                              {'Name': 'set attributes', 'Args': {'Groups' : group_names}}],
                   'Options': {'IgnoreException':False}}
      try:
         domain.alter(options = bulk_opts)  
         self.fail('need alter failed!')       
      except SDBBaseError as e:
         self.assertEqual(e.code, -6)       

      # check after bulk fail
      self.check_domain_cs(expect_bulk_attributes, condition = {'Name' : self.domain_name})         
      
   def tearDown(self):
      self.db.drop_domain(self.domain_name)
      self.db.disconnect()
      
   def check_domain_cs(self, expect_domain_cs, **kwargs):
      act_domain_cs = get_domain_attrbutes(self.db, **kwargs)
      self.assertEqual(len(act_domain_cs), len(expect_domain_cs))
      self.assertEqual(len(act_domain_cs), 1)
      self.assertEqual(act_domain_cs[0]['AutoSplit'], expect_domain_cs[0]['AutoSplit'])
      self.assertListEqualUnordered(act_domain_cs[0]['Groups'], expect_domain_cs[0]['Groups'])
         
# @decription: test domain set attributes
# @testlink:   seqDB-15211
# @interface:  set_attributes(self, options)
# @author:     liuxiaoxuan 2018-04-25

import unittest
from pysequoiadb.error import (SDBBaseError,SDBEndOfCursor)
from alter.commlib import *
from lib import testlib

class TestAlterDomain15211(testlib.SdbTestBase):
   def setUp(self):
      # skip standalone
      if testlib.is_standalone():
         self.skipTest('run mode is standalone')
      # skip one group   
      if 2 > testlib.get_data_group_num():
         self.skipTest('less than two groups')   
      self.domain_name = 'testattributesdomain15211'
      
   def test_domain_set_attributes_15211(self):
   
      # drop domain if exists
      try:
         self.db.drop_domain(self.domain_name)
      except SDBBaseError as e:     		
         self.assertEqual(e.code, -214)
   
      # get groups
      groups = testlib.get_data_groups()
      group_size = len(groups)
      group_names = [groups[i]['GroupName'] for i in range(group_size - 1)]
      
      # create domain
      domain = self.db.create_domain(self.domain_name, {'AutoSplit': False, "Groups": group_names})
      
      # check result before alter
      expect_domain = [{'Name': self.domain_name, 'AutoSplit': False, 'Groups': group_names}]
      self.check_domain(expect_domain, condition = {'Name': self.domain_name})
      
      # alter attributes
      group_names.append(groups[group_size - 1]['GroupName'])
      alter_opts = { 'AutoSplit' : True, 'Groups': group_names}
      domain.set_attributes(options = alter_opts)
     
      # check result after alter
      expect_domain = [{'Name': self.domain_name, 'AutoSplit': True, 'Groups': group_names}]
      self.check_domain(expect_domain, condition = {'Name': self.domain_name}) 
      
      # alter domain name, must fail
      try:
         domain.set_attributes(options = {'Name' : 'newdomainname'})
         self.fail('need alter fail')
      except SDBBaseError as e:     		
         self.assertEqual(e.code, -6)
      
      # check result after fail
      expect_domain = [{'Name': self.domain_name, 'AutoSplit': True, 'Groups': group_names}]
      self.check_domain(expect_domain, condition = {'Name': self.domain_name})
        
   def tearDown(self):
      # remove domain
      self.db.drop_domain(self.domain_name)
      self.db.disconnect()

   def check_domain(self, expect_domain, **kwargs):
      act_domain = get_domain_attrbutes(self.db, **kwargs)
      self.assertEqual(len(act_domain), len(expect_domain))
      self.assertEqual(len(act_domain), 1)
      self.assertEqual(act_domain[0]['AutoSplit'], expect_domain[0]['AutoSplit'])
      self.assertListEqualUnordered(act_domain[0]['Groups'], expect_domain[0]['Groups'])
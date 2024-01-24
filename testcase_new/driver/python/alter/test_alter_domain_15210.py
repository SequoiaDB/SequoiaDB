# @decription: test alter domain
# @testlink:   seqDB-15210
# @interface:  add_groups(self, options)
# @interface:  set_groups(self, options)
# @interface:  remove_groups(self, options)
# @author:     liuxiaoxuan 2018-04-25

import unittest
from pysequoiadb.error import (SDBBaseError,SDBEndOfCursor)
from alter.commlib import *
from lib import testlib

class TestAlterDomain15210(testlib.SdbTestBase):
   def setUp(self):
      # skip standalone
      if testlib.is_standalone():
         self.skipTest('run mode is standalone')
      # skip less than three group
      if 3 > testlib.get_data_group_num():
         self.skipTest('less than three groups')
         
      self.domain_name = 'testalterdomain15210'
      
   def test_alter_domain_15210(self):
   
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
      self.db.create_domain(self.domain_name, { "Groups": group_names })
      
      # create cs
      cs_name = 'testalterdomain15210_cs'
      self.db.create_collection_space(cs_name, {'Domain': self.domain_name})
      
      # check result
      expect_domains = [{'Groups': group_names}]
      self.check_domain(expect_domains, condition = {'Name': self.domain_name})
      
      # add group in domain
      domain = self.db.get_domain(self.domain_name)
      domain.add_groups( options = {'Groups': groups[group_size - 1]['GroupName'] })
      group_names.append(groups[group_size - 1]['GroupName'])
      
      # check result
      expect_domains = [{'Groups': group_names}]
      self.check_domain(expect_domains, condition = {'Name': self.domain_name})
      
      # set group in domain
      del group_names[0]
      domain.set_groups(options = { "Groups": group_names })
      
      # check result
      expect_domains = [{'Groups': group_names}]
      self.check_domain(expect_domains, condition = {'Name': self.domain_name})
      
      # remove group in domain
      domain.remove_groups({ "Groups": [group_names[0]]})
      del group_names[0]
      
      # check domain
      expect_domains = [{'Groups': group_names}]
      self.check_domain(expect_domains, condition = {'Name': self.domain_name})    
		 
   def tearDown(self):
      # drop cs
      self.db.drop_collection_space('testalterdomain15210_cs')
      # remove domain
      self.db.drop_domain(self.domain_name)
      self.db.disconnect()

   def check_domain(self, expect_domain, **kwargs):
      act_domain = get_domain_attrbutes(self.db, **kwargs)
      self.assertEqual(len(act_domain), len(expect_domain))
      self.assertEqual(len(act_domain), 1)
      self.assertListEqualUnordered(act_domain[0]['Groups'], expect_domain[0]['Groups'])
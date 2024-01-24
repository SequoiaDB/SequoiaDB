# @decription: metadata, domain
# @testlink:   seqDB-13482
# @interface:  create_domain(domain_name,options = None) 
#              is_domain_existing(domain_name)
#              list_domains(kwargs) 
#              domain.name
#              get_domain(domain_name)
#              domain.alter(options)
#              dropDomain (String domainName)
# @author:     liuxiaoxuan 2017-11-21

import unittest
from lib import testlib
from lib import sdbconfig
from pysequoiadb import client
from pysequoiadb.error import SDBBaseError

class TestMetedata13482(testlib.SdbTestBase):
   def setUp(self):
      # check standalone
      if testlib.is_standalone():
         self.skipTest('current environment is standalone')
      self.domain_name = "domain_13482"
      
   def test_metedata_13482(self):
      # get data groups
      data_groups = testlib.get_data_groups()
      
      # create domain with options groups
      group1 = data_groups[0]['GroupName']
      options = {'Groups' : [group1]}
      self.db.create_domain(self.domain_name, options)
      
      # check domain
      is_domain_exist = self.db.is_domain_existing(self.domain_name)
      self.assertTrue(is_domain_exist)
      
      domain = self.db.get_domain(self.domain_name)
      act_domain_name = domain.name 
      self.assertEqual(act_domain_name, self.domain_name)
      
      condition = {'Name' : self.domain_name}
      selector = {'Name' : {'$include' : 1}}
      order_by = {'_id' : 1}
      cur = self.db.list_domains(condition = condition, selector = selector, order_by = order_by)
      domain_result = testlib.get_all_records_noid(cur)
      self.assertEqual(1, len(domain_result))
      self.assertEqual(self.domain_name, domain_result[0]['Name'])
      
      # alter domain
      alter_option = {'AutoSplit' : True}
      domain.alter(alter_option)
      
      # check domain
      cur = self.db.list_domains(condition = condition, order_by = order_by)
      domain_result = testlib.get_all_records_noid(cur)
      print("GroupName: " + domain_result[0]['Groups'][0]["GroupName"])
      self.assertEqual(1, len(domain_result))
      self.assertEqual(self.domain_name, domain_result[0]['Name'])
      self.assertEqual(group1, domain_result[0]['Groups'][0]["GroupName"])
      self.assertTrue(domain_result[0]['AutoSplit']) 
         
      # drop domain  
      self.db.drop_domain(self.domain_name)      
      
      # check domain
      is_domain_exist = self.db.is_domain_existing(self.domain_name)
      self.assertFalse(is_domain_exist)
      
      try:
         self.db.get_domain(self.domain_name)
         self.fail("delete domain fail") 
      except BaseException as e:
         if not e.code == -214:
            self.fail("delete domain fail: " + str(e))
            
      #create same domain with no option
      self.db.create_domain(self.domain_name)
      
      # check domain
      is_domain_exist = self.db.is_domain_existing(self.domain_name)
      self.assertTrue(is_domain_exist)
      
      domain = self.db.get_domain(self.domain_name)
      act_domain_name = domain.name 
      self.assertEqual(act_domain_name, self.domain_name)
      
      cur = self.db.list_domains()
      rec = testlib.get_all_records_noid(cur)
      domain_result = list()
      for r in rec:
         domain_result.append(str(r['Name']))
      self.assertIn(self.domain_name, domain_result, 'not find domain') 
      
      # alter domain
      alter_option = {'Groups' : [group1], 'AutoSplit' : True}
      domain.alter(alter_option)
         
      # check domain
      cur = self.db.list_domains()
      rec = testlib.get_all_records_noid(cur)
      domain_result = dict()
      for r in rec:
         if self.domain_name == str(r['Name']):
            domain_result = {'Name': str(r['Name']), 'Group': r['Groups'][0]['GroupName'], 'AutoSplit': True}
      self.assertEqual(self.domain_name, domain_result['Name'], 'not find domain') 
      self.assertEqual(group1, domain_result['Group']) 
      self.assertTrue(domain_result['AutoSplit']) 
      
      # drop domain  
      self.db.drop_domain(self.domain_name)      
      
      # check domain
      is_domain_exist = self.db.is_domain_existing(self.domain_name)
      self.assertFalse(is_domain_exist) 
      
      # 13484, drop domain not exist
      try:
         self.db.drop_domain(self.domain_name) 
         self.fail("domain does not exist") 
      except SDBBaseError as e:
         if not e.code == -214:
            self.fail("drop domain fail: " + str(e))

      #create same domain with options AutoSplit
      try:
         options = {'AutoSplit' : True}
         self.db.create_domain(self.domain_name, options)
         self.fail("create domain with only AutoSplit fail") 
      except:
         pass  
     
   def tearDown(self):
      try:
         self.db.drop_domain(self.domain_name)
      except SDBBaseError as e:
         if e.code != -214:
            self.fail("drop domain fail when teardown: " + str(e))
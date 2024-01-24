# @decription: test list domains
# @testlink:   seqDB-12504
# @interface:  get_list(self,list_type,kwargs)
# @author:     liuxiaoxuan 2017-02-22

import unittest
import datetime
from pysequoiadb.error import (SDBBaseError, SDBEndOfCursor)
from lib import testlib

domain_name = 'domain12504'
sdb_list_domains = 9
class TestListDomains12504(testlib.SdbTestBase):
   def setUp(self):
      # skip standalone
      if testlib.is_standalone():
         self.skipTest('run mode is standalone')
      try:
         self.db.drop_domain(domain_name)  
      except SDBBaseError as e:
         if -214 != e.code:
            self.fail('set up fail: ' + str(e))

   def test_list_domains_12504(self):
      # create domain
      self.db.create_domain(domain_name)
      
      # list 9
      expect_result = {'Name': domain_name}
      act_result = self.get_list_domains()
      # check list
      self.check_list(expect_result, act_result)
		
   def tearDown(self):
      self.db.drop_domain(domain_name) 

   def get_list_domains(self):
      act_result = None
      try:
         cursor = self.db.get_list(sdb_list_domains)
         # get result
         act_result = self.get_actual_result(cursor)
      except SDBBaseError as e:
         self.fail('get list domains fail: ' + str(e))
      return act_result
            
   def get_actual_result(self,cursor):
      item = {}
      while True:
         try:
            rec = cursor.next()
            if domain_name == rec['Name']:
               item = {'Name': rec['Name']}
               cursor.close()
               break
         except SDBEndOfCursor:
            cursor.close()
            break
      return item
      
   def check_list(self, expect_result, act_result):
      self.assertEqual(expect_result['Name'], act_result['Name'])   
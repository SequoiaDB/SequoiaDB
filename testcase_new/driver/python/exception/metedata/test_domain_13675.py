# @decription: create exist dommain, get/remove non exist domain
# @testlink:   seqDB-13675
# @author:     liuxiaoxuan 2017-12-07

from lib import testlib
from pysequoiadb.error import (SDBBaseError, SDBError)

class DomainException13675(testlib.SdbTestBase):
   def setUp(self):
      self.domain_name = "domain13675"
      if testlib.is_standalone():
         self.skipTest('skip standalone') 

   def test_comain_13675(self):
      # create non exist domain
      self.db.create_domain(self.domain_name)
      
      # check domain created success
      self.check_create_domain_success(self.domain_name)
      
      # create exist domain
      self.check_error_create_domain(self.domain_name)
      
      # drop exist domain
      self.db.drop_domain(self.domain_name)
      
      # check domain removed success
      self.check_remove_domain_success(self.domain_name)
      
      # get non exist domain
      self.check_error_get_domain(self.domain_name)
      
      # remove non exist domain
      self.check_error_remove_domain(self.domain_name)
      
   def check_create_domain_success(self, domain_name):
      try:
         cursor = self.db.list_domains()
         act_domains = testlib.get_all_records_noid(cursor)
         
         expect_domain = {'Name': domain_name}
         self.assertIn(expect_domain, act_domains)
            
      except SDBBaseError as e:
         self.fail("check domain fail: " + str(e))
         
   def check_remove_domain_success(self, domain_name):
      try:
         cursor = self.db.list_domains()
         act_domains = testlib.get_all_records_noid(cursor)
         
         expect_domain = {'Name': domain_name}
         self.assertNotIn(expect_domain, act_domains)
      except SDBBaseError as e:
         self.fail("check domain fail: " + str(e))

   def check_error_create_domain(self, domain_name):
      try:
         self.db.create_domain(domain_name)
         self.fail("NEED SDB ERROR")
      except SDBError as e:
         self.assertEqual(e.code, -215)
         self.assertEqual(e.detail, "Failed to create domain: " + domain_name)   
         
   def check_error_get_domain(self, domain_name):
      try:
         self.db.get_domain(domain_name)
         self.fail("NEED SDB ERROR")
      except SDBError as e:
         self.assertEqual(e.code, -214)
         self.assertEqual(e.detail, "Failed to get domain: " + domain_name)  

   def check_error_remove_domain(self, domain_name):
      try:
         self.db.drop_domain(domain_name)
         self.fail("NEED SDB ERROR")
      except SDBError as e:
         self.assertEqual(e.code, -214)
         self.assertEqual(e.detail, "Failed to drop domain: " + domain_name)          

   def tearDown(self):
      if self.should_clean_env():
         try:
            self.db.drop_domain(self.domain_name)
         except SDBBaseError as e:
            if -214 != e.code:
               self.fail("teardown fail: " + str(e))

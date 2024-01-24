# @decription: test list procedures
# @testlink:   seqDB-12504
# @interface:  get_list(self,list_type,kwargs)
# @author:     liuxiaoxuan 2017-02-22

import unittest
import datetime
from pysequoiadb.error import (SDBBaseError, SDBEndOfCursor)
from lib import testlib

sdb_list_procedures = 8
class TestListProcedures12504(testlib.SdbTestBase):
   def setUp(self):
      # skip standalone
      if testlib.is_standalone():
         self.skipTest('run mode is standalone')
      try:
         self.db.remove_procedure('sum12504')  
      except SDBBaseError as e:
         if -233 != e.code:
            self.fail('set up fail: ' + str(e))

   def test_list_procedures_12504(self):
      # create procedure
      code = 'function sum12504(x,y) { return x+y;}'
      self.db.create_procedure(code)
      
      # list 8
      expect_result = {'name': 'sum12504'}
      act_result = self.get_list_procedures()
      # check list
      self.check_list(expect_result, act_result)
		
   def tearDown(self):
      name = 'sum12504'
      self.db.remove_procedure(name)

   def get_list_procedures(self):
      act_result = None
      try:
         cursor = self.db.get_list(sdb_list_procedures)
         # get result
         act_result = self.get_actual_result(cursor)
      except SDBBaseError as e:
         self.fail('get list procedures fail: ' + str(e))
      return act_result
            
   def get_actual_result(self,cursor):
      item = {}
      while True:
         try:
            rec = cursor.next()
            if 'sum12504' == rec['name']:
               item = {'name': rec['name']}
               cursor.close()
               break
         except SDBEndOfCursor:
            cursor.close()
            break
      return item
      
   def check_list(self, expect_result, act_result):
      self.assertEqual(expect_result['name'], act_result['name'])   

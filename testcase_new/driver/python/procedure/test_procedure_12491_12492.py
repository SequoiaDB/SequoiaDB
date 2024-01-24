# @decription: test store procedure
# @testlink:   seqDB-12491/seqDB-12492
# @interface:  create_procedure(self,code)
#              list_procedures(self,condition)
#              eval_procedure(self,name)
#              remove_procedure(self,name)
# @author:     liuxiaoxuan 2017-9-08

import unittest
import datetime
from pysequoiadb.error import (SDBTypeError, SDBBaseError, SDBEndOfCursor, SDBError)
from bson.code import Code
from lib import testlib

class TestProcedure12491(testlib.SdbTestBase):
   def setUp(self):
      if testlib.is_standalone():
         self.skipTest('current environment is standalone')  

   def test_procedure_12491(self):
      # check create result
      code = 'function sum12491(x,y) { return x + y; }'
      self.check_create_procedure(code)

      # check list result
      condition = {'name': 'sum12491'}
      expectResult = {'name': 'sum12491','code': Code(code)}
      self.check_list_procedure(condition,expectResult)

      # check eval
      name = 'sum12491(1,2)'
      expectResult = 3
      self.check_eval_procedure(name,expectResult)

      # check remove result
      name = 'sum12491'
      self.check_remove_procedure(name)

      # check remove not exist procedure(seqDB-12492)
      names = ["sum12491", "find", "remove", "update", "insert"]
      self.check_remove_none_procedure(names)

   def tearDown(self):
      if self.should_clean_env():
         name = 'sum12491'
         try:
            self.db.remove_procedure(name)
         except SDBBaseError as e:
            if -233 != e.code:
               self.fail("teardown fail,errmsg: " + str(e))

   def check_create_procedure(self,code):
      try:
         self.db.create_procedure(code)
      except SDBBaseError as e:
         self.fail("create procedure fail: " + str(e))

   def check_list_procedure(self,cond,expectResult):
      try:
         cursor = self.db.list_procedures(condition = cond)
         actResult = cursor.next()
         self.assertEqual(expectResult['name'],actResult['name'])
         self.assertEqual(expectResult['code'],actResult['func'])
      except SDBBaseError as e:
         self.fail("list procedure fail: " + str(e))

   def check_eval_procedure(self,name,expectResult):
      try:
         cursor = self.db.eval_procedure(name)
         actResult = cursor.next()
         self.assertEqual(expectResult, actResult['value'])
      except SDBBaseError as e:
         self.fail("eval procedure fail: " + str(e))

   def check_remove_procedure(self,name):
      try:
         self.db.remove_procedure(name)
         # check result
         result = 0
         cursor = self.db.list_procedures(condition = {'name': name})
         try:
             cursor.next()
             result = result + 1
         except SDBEndOfCursor:
             self.assertEqual(0, result, 'remove procedure fail')
      except SDBBaseError as e:
         self.fail("remove procedure fail: " + str(e))

   def check_remove_none_procedure(self,names):
      for i in range(len(names)):
         try:
            self.db.remove_procedure(names[i])
            self.fail("NEED REMOVE PROCEDURE FAIL")
         except SDBBaseError as e:
            # procedure not exist
            self.assertEqual(-233, e.code, "remove procedure(" + names[i] + ") fail, msg: " + e.detail)

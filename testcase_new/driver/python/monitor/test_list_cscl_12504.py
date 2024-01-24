# @decription: test list cscl
# @testlink:   seqDB-12504
# @interface:  get_list(self,list_type,kwargs)
# @author:     liuxiaoxuan 2017-9-08

import unittest
import datetime
from pysequoiadb.error import (SDBTypeError, SDBBaseError, SDBEndOfCursor, SDBError)
from lib import testlib

sdb_list_collections = 4
sdb_list_collectionspaces = 5
class TestListCollections12504(testlib.SdbTestBase):
   def setUp(self):
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)

   def test_list_collections_12504(self):

      cl_full_name = self.cl_name_qualified
      condition_cl = [{"Name": cl_full_name} , None]
      condition_cs = [{"Name": self.cs_name} , None]

      # get list cl with option
      expect_result_cl = [cl_full_name]
      act_result_cl = self.get_list_collections(condition_cl[0])
      
      # check list
      self.check_list(expect_result_cl, act_result_cl)
      
      # get list cs with option
      expect_result_cs = [self.cs_name]
      act_result_cs = self.get_list_collectionspaces(condition_cs[0])
      
      # check list
      self.check_list(expect_result_cs, act_result_cs)

      # get list cl without option
      act_result_cl = self.get_list_collections(condition_cl[1])
      
      # check list
      self.check_list(expect_result_cl, act_result_cl)
      
      # get list cs without option
      act_result_cs = self.get_list_collectionspaces(condition_cs[1])
      
      # check list
      self.check_list(expect_result_cs, act_result_cs)

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)

   def get_list_collections(self, cond):
      act_result = []
      try:
         if(cond == None):
            cursor = self.db.get_list(sdb_list_collections)
         else:
            cursor = self.db.get_list(sdb_list_collections, condition = cond)

         act_result = self.get_act_result(cursor)
      except SDBBaseError as e:
         self.fail('get list fail: ' + str(e))
      return act_result
      
   def get_list_collectionspaces(self, cond):
      act_result = []
      try:
         if(cond == None):
            cursor = self.db.get_list(sdb_list_collectionspaces)
         else:
            cursor = self.db.get_list(sdb_list_collectionspaces, condition = cond)

         act_result = self.get_act_result(cursor)
      except SDBBaseError as e:
         self.fail('get list fail: ' + str(e))
      return act_result

   def get_act_result(self, cursor):
      act_result = []
      while True:
         try:
            rec = cursor.next()
            act_result.append(rec['Name'])
         except SDBEndOfCursor:
            cursor.close()
            break
      return act_result

   def check_list(self, expect, act):
      for x in expect:
         self.assertIn(x, act, x + " not in " + str(act))
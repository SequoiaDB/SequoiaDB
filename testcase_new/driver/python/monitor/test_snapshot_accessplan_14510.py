# @decription: test accessplan snapshot
# @testlink:   seqDB-14510
# @interface:  get_snapshot(self,snap_type,kwargs)
# @author:     liuxiaoxuan 2017-9-08

import unittest
import datetime
from pysequoiadb.error import (SDBBaseError, SDBEndOfCursor, SDBError)
import time
from lib import testlib

snap_type_11= 11
class TestSnapshot14510(testlib.SdbTestBase):
   def setUp(self):
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)

   def test_snapshot_14510(self):
      
      # insert datas
      insert_nums = 1000
      self.insert_datas(insert_nums)
      
      # create index
      self.cl.create_index({'a' : 1}, 'a')
      
      # query
      cond = {'a' : 2000}
      select = { 'a': { '$include': 1 } }
      sort = {'_id': 1}
      self.query(condition = cond, selector = select, order_by = sort)
      
      # get snapshot accessplan
      expect_accessplan = [{'ScanType': 'ixscan', 'IndexName': 'a', 'ReturnNum': insert_nums}]
      opt = {'Collection': self.cs_name + '.' + self.cl_name}
      act_accessplan = self.get_snapshot_accessplan(opt)

      # check snapshot
      self.check_snapshot(expect_accessplan, act_accessplan)  
		
   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)

   def query(self, **kwargs):
      try:
         cursor = self.cl.query(**kwargs)
         while True:
            try:
               cursor.next()
            except SDBEndOfCursor:
               cursor.close()
               break
      except SDBBaseError as e:
         self.fail('query fail: ' + str(e))
  
   def get_snapshot_accessplan(self, opt):
      act_accessplan = list()
      try:
         cursor = self.db.get_snapshot(snap_type_11, condition = opt)
         # get result
         while True:
            try:
               rec = cursor.next()
               act_accessplan.append({'ScanType': rec['ScanType'], 'IndexName': rec['IndexName'], 'ReturnNum': rec['MaxTimeSpentQuery']['ReturnNum']})
            except SDBEndOfCursor:
               cursor.close()
               break
      except SDBBaseError as e:
         self.fail('get snapshot accessplan fail: ' + str(e))
      return act_accessplan  

   def insert_datas(self, insert_nums):
      flag = 0
      doc = []
      for i in range(0, insert_nums):
         doc.append({ "a": i})
         doc.append({ "a": 2000})
      try:
         self.cl.bulk_insert(flag, doc)
      except SDBError as e:
         self.fail('insert fail: ' + str(e))
      
   def check_snapshot(self, expect_accessplan, act_accessplan):
      expect_explain = self.get_sort_result(expect_accessplan)
      act_explain = self.get_sort_result(act_accessplan)
      self.assertListEqualUnordered(expect_accessplan, act_accessplan)   
         
   def get_sort_result(self, accessplan_result):
      new_result = list()
      for x in accessplan_result:
         item = sorted(x.items())
         new_result.append(item) 
      return new_result
# @decription: analyze with indexes
# @testlink:   seqDB-14217
# @interface:  analyze(self,options)
# @author:     liuxiaoxuan 2018-01-30

from lib import testlib
from analyze.commlib import *
from pysequoiadb.error import (SDBBaseError, SDBEndOfCursor)

class TestAnalyzeIndex14217(testlib.SdbTestBase):
   def setUp(self):
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      # create cs
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name, {'ReplSize': 0})
      
   def test_analyze_index_14217(self):
      
      # insert datas
      insert_nums = 10000
      same_value = 0
      self.insert_diff_datas(self.cl, insert_nums)
      self.insert_same_datas(self.cl, insert_nums, same_value)
      
      # create indexs
      self.cl.create_index({'a':1}, 'a', False, False)
      self.cl.create_index({'a0':1}, 'a0', False, False)
      self.cl.create_index({'a1':1}, 'a1', False, False)
        
      # check explain before analyze
      cursor1 = self.cl.explain(condition = {'a' : 0}, options = {'Run' : True})
      cursor2 = self.cl.explain(condition = {'a0' : 0}, options = {'Run' : True})
      cursor3 = self.cl.explain(condition = {'a1' : 0}, options = {'Run' : True})
        
      act_explain1 = get_common_explain(cursor1)
      act_explain2 = get_common_explain(cursor2)
      act_explain3 = get_common_explain(cursor3)
      
      exp_explain1 = [{'ScanType': 'ixscan', 'IndexName': 'a', 'ReturnNum': insert_nums}]
      exp_explain2 = [{'ScanType': 'ixscan', 'IndexName': 'a0', 'ReturnNum': insert_nums}]
      exp_explain3 = [{'ScanType': 'ixscan', 'IndexName': 'a1', 'ReturnNum': insert_nums}]
   
      self.check_explain(exp_explain1, act_explain1)
      self.check_explain(exp_explain2, act_explain2)
      self.check_explain(exp_explain3, act_explain3)
      
      print('------check explain success before analyze------')
      
      # analyze indexes
      self.db.analyze(options = {'Collection': self.cs_name + "." + self.cl_name, 'Index': 'a'})
      self.db.analyze(options = {'Collection': self.cs_name + "." + self.cl_name, 'Index': 'a0'})
               
      # check explain before analyze
      cursor1 = self.cl.explain(condition = {'a' : 0}, options = {'Run' : True})
      cursor2 = self.cl.explain(condition = {'a0' : 0}, options = {'Run' : True})
      cursor3 = self.cl.explain(condition = {'a1' : 0}, options = {'Run' : True})
        
      act_explain1 = get_common_explain(cursor1)
      act_explain2 = get_common_explain(cursor2)
      act_explain3 = get_common_explain(cursor3)
      
      exp_explain1 = [{'ScanType': 'tbscan', 'IndexName': '', 'ReturnNum': insert_nums}]
      exp_explain2 = [{'ScanType': 'tbscan', 'IndexName': '', 'ReturnNum': insert_nums}]
      exp_explain3 = [{'ScanType': 'ixscan', 'IndexName': 'a1', 'ReturnNum': insert_nums}]
   
      self.check_explain(exp_explain1, act_explain1)
      self.check_explain(exp_explain2, act_explain2)
      self.check_explain(exp_explain3, act_explain3)
      
      print('------check explain success after analyze indexes------') 
      
      # analyze index default
      self.db.analyze(options = {'Collection': self.cs_name + "." + self.cl_name, 'Index': 'a0', 'Mode': 3})
                           
      # check explain before analyze
      cursor1 = self.cl.explain(condition = {'a' : 0}, options = {'Run' : True})
      cursor2 = self.cl.explain(condition = {'a0' : 0}, options = {'Run' : True})
      cursor3 = self.cl.explain(condition = {'a1' : 0}, options = {'Run' : True})
        
      act_explain1 = get_common_explain(cursor1)
      act_explain2 = get_common_explain(cursor2)
      act_explain3 = get_common_explain(cursor3)
      
      exp_explain1 = [{'ScanType': 'tbscan', 'IndexName': '', 'ReturnNum': insert_nums}]
      exp_explain2 = [{'ScanType': 'ixscan', 'IndexName': 'a0', 'ReturnNum': insert_nums}]
      exp_explain3 = [{'ScanType': 'ixscan', 'IndexName': 'a1', 'ReturnNum': insert_nums}]
   
      self.check_explain(exp_explain1, act_explain1)
      self.check_explain(exp_explain2, act_explain2)
      self.check_explain(exp_explain3, act_explain3)
      
      # analyze rest indexes default
      self.db.analyze(options = {'Collection': self.cs_name + "." + self.cl_name, 'Index': 'a', 'Mode': 3})
                           
      # check explain before analyze
      cursor1 = self.cl.explain(condition = {'a' : 0}, options = {'Run' : True})
      cursor2 = self.cl.explain(condition = {'a0' : 0}, options = {'Run' : True})
      cursor3 = self.cl.explain(condition = {'a1' : 0}, options = {'Run' : True})
        
      act_explain1 = get_common_explain(cursor1)
      act_explain2 = get_common_explain(cursor2)
      act_explain3 = get_common_explain(cursor3)
      
      exp_explain1 = [{'ScanType': 'ixscan', 'IndexName': 'a', 'ReturnNum': insert_nums}]
      exp_explain2 = [{'ScanType': 'ixscan', 'IndexName': 'a0', 'ReturnNum': insert_nums}]
      exp_explain3 = [{'ScanType': 'ixscan', 'IndexName': 'a1', 'ReturnNum': insert_nums}]
   
      self.check_explain(exp_explain1, act_explain1)
      self.check_explain(exp_explain2, act_explain2)
      self.check_explain(exp_explain3, act_explain3)
      
      print('------check explain success after default analyze indexes------') 

   def tearDown(self):
      if self.should_clean_env():
         testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
         
   def insert_diff_datas(self, cl, insert_nums):
      flag = 0
      doc = []
      for i in range(1, insert_nums + 1):
         doc.append({"a": i,"a0": i,"a1": i,"a2": i,"a3": i,"a4": i,"a5": i,
                     "a6": i,"a7": i,"a8": i,"a9": i})
      try:
         cl.bulk_insert(flag, doc)
      except SDBBaseError as e:   
         self.fail('insert fail: ' + str(e))
         
   def insert_same_datas(self, cl, insert_nums, same_value):
      flag = 0
      doc = []
      for i in range(0, insert_nums):
         doc.append({"a": same_value,"a0": same_value,"a1": same_value,"a2": same_value,
                     "a3": same_value,"a4": same_value,"a5": same_value,"a6": same_value,
                     "a7": same_value,"a8": same_value,"a9": same_value})
      try:
         cl.bulk_insert(flag, doc)
      except SDBBaseError as e:   
         self.fail('insert fail: ' + str(e))
         
   def check_explain(self, expect_explain, act_explain):
      expect_explain = get_sort_result(expect_explain)
      act_explain = get_sort_result(act_explain)
      self.assertListEqualUnordered(expect_explain, act_explain)
 
# @decription: analyze with group
# @testlink:   seqDB-14218
# @interface:  analyze(self,options)
# @author:     liuxiaoxuan 2018-01-30

from lib import testlib
from analyze_sync.commlib import *
from pysequoiadb.error import (SDBBaseError, SDBEndOfCursor)

class TestAnalyzeGroup14218(testlib.SdbTestBase):
   def setUp(self):
      # skip standalone
      if testlib.is_standalone():
         self.skipTest('run mode is standalone')
      # skip one group
      if 2 > testlib.get_data_group_num():
         self.skipTest('less than two groups')
         
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      # create cs
      self.cs = self.db.create_collection_space(self.cs_name)
      
      
   def test_analyze_group_14218(self):
      # get groups
      groups = testlib.get_data_groups()
      src_groupname = groups[0]['GroupName']
      dest_groupname = groups[1]['GroupName']
      
      # create cl
      options = {'Group': src_groupname, 'ShardingKey' : {'a' : 1}, 'ShardingType': 'range', 'ReplSize': 0}
      self.cl = self.cs.create_collection(self.cl_name, options)
      
      # insert datas
      insert_nums = 10000
      same_value1 = 0
      same_value2 = 9999
      self.insert_same_datas(self.cl, insert_nums, same_value1)
      self.insert_same_datas(self.cl, insert_nums, same_value2)
      
      # split cl
      self.cl.split_by_percent(src_groupname, dest_groupname, 50)
      
      # create index
      self.cl.create_index({'a0':1}, 'a0', False, False)
        
      # check explain before analyze
      cursor1 = self.cl.explain(condition = {'a' : 0}, options = {'Run' : True})
      cursor2 = self.cl.explain(condition = {'a0' : 0}, options = {'Run' : True})
      cursor3 = self.cl.explain(condition = {'a' : 9999}, options = {'Run' : True})
      cursor4 = self.cl.explain(condition = {'a0' : 9999}, options = {'Run' : True})
        
      act_explain1 = get_split_explain(cursor1)
      act_explain2 = get_split_explain(cursor2)
      act_explain3 = get_split_explain(cursor3)
      act_explain4 = get_split_explain(cursor4)
      
      exp_explain1 = [{'GroupName': src_groupname, 'ScanType': 'ixscan', 'IndexName': '$shard', 'ReturnNum': insert_nums}]
      exp_explain2 = [{'GroupName': src_groupname, 'ScanType': 'ixscan', 'IndexName': 'a0', 'ReturnNum': insert_nums},
                      {'GroupName': dest_groupname, 'ScanType': 'ixscan', 'IndexName': 'a0', 'ReturnNum': 0}]
      exp_explain3 = [{'GroupName': dest_groupname, 'ScanType': 'ixscan', 'IndexName': '$shard', 'ReturnNum': insert_nums}]
      exp_explain4 = [{'GroupName': dest_groupname, 'ScanType': 'ixscan', 'IndexName': 'a0', 'ReturnNum': insert_nums},
                      {'GroupName': src_groupname, 'ScanType': 'ixscan', 'IndexName': 'a0', 'ReturnNum': 0}]

      self.check_explain(exp_explain1, act_explain1)
      self.check_explain(exp_explain2, act_explain2)
      self.check_explain(exp_explain3, act_explain3)
      self.check_explain(exp_explain4, act_explain4)
      
      print('------check explain success before analyze------')
      
      # analyze src group
      self.db.analyze(options = {'GroupName': src_groupname})
                           
      # check explain after analyze
      cursor1 = self.cl.explain(condition = {'a' : 0}, options = {'Run' : True})
      cursor2 = self.cl.explain(condition = {'a0' : 0}, options = {'Run' : True})
      cursor3 = self.cl.explain(condition = {'a' : 9999}, options = {'Run' : True})
      cursor4 = self.cl.explain(condition = {'a0' : 9999}, options = {'Run' : True})
        
      act_explain1 = get_split_explain(cursor1)
      act_explain2 = get_split_explain(cursor2)
      act_explain3 = get_split_explain(cursor3)
      act_explain4 = get_split_explain(cursor4)
      
      exp_explain1 = [{'GroupName': src_groupname, 'ScanType': 'tbscan', 'IndexName': '', 'ReturnNum': insert_nums}]
      exp_explain2 = [{'GroupName': src_groupname, 'ScanType': 'tbscan', 'IndexName': '', 'ReturnNum': insert_nums},
                      {'GroupName': dest_groupname, 'ScanType': 'ixscan', 'IndexName': 'a0', 'ReturnNum': 0}]
      exp_explain3 = [{'GroupName': dest_groupname, 'ScanType': 'ixscan', 'IndexName': '$shard', 'ReturnNum': insert_nums}]
      exp_explain4 = [{'GroupName': dest_groupname, 'ScanType': 'ixscan', 'IndexName': 'a0', 'ReturnNum': insert_nums},
                      {'GroupName': src_groupname, 'ScanType': 'ixscan', 'IndexName': 'a0', 'ReturnNum': 0}]

      self.check_explain(exp_explain1, act_explain1)
      self.check_explain(exp_explain2, act_explain2)
      self.check_explain(exp_explain3, act_explain3)
      self.check_explain(exp_explain4, act_explain4)
      
      print('------check explain success after analyze src group------') 
      
      # analyze dest group
      self.db.analyze(options = {'GroupName': dest_groupname})
                           
      # check explain after analyze 
      cursor1 = self.cl.explain(condition = {'a' : 0}, options = {'Run' : True})
      cursor2 = self.cl.explain(condition = {'a0' : 0}, options = {'Run' : True})
      cursor3 = self.cl.explain(condition = {'a' : 9999}, options = {'Run' : True})
      cursor4 = self.cl.explain(condition = {'a0' : 9999}, options = {'Run' : True})
        
      act_explain1 = get_split_explain(cursor1)
      act_explain2 = get_split_explain(cursor2)
      act_explain3 = get_split_explain(cursor3)
      act_explain4 = get_split_explain(cursor4)
      
      exp_explain1 = [{'GroupName': src_groupname, 'ScanType': 'tbscan', 'IndexName': '', 'ReturnNum': insert_nums}]
      exp_explain2 = [{'GroupName': src_groupname, 'ScanType': 'tbscan', 'IndexName': '', 'ReturnNum': insert_nums},
                      {'GroupName': dest_groupname, 'ScanType': 'ixscan', 'IndexName': 'a0', 'ReturnNum': 0}]
      exp_explain3 = [{'GroupName': dest_groupname, 'ScanType': 'tbscan', 'IndexName': '', 'ReturnNum': insert_nums}]
      exp_explain4 = [{'GroupName': dest_groupname, 'ScanType': 'tbscan', 'IndexName': '', 'ReturnNum': insert_nums},
                      {'GroupName': src_groupname, 'ScanType': 'ixscan', 'IndexName': 'a0', 'ReturnNum': 0}]

      self.check_explain(exp_explain1, act_explain1)
      self.check_explain(exp_explain2, act_explain2)
      self.check_explain(exp_explain3, act_explain3)
      self.check_explain(exp_explain4, act_explain4)
      
      print('------check explain success after analyze dest group------') 

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
 
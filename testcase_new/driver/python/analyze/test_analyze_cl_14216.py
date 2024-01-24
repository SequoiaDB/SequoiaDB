# @decription: analyze with cl
# @testlink:   seqDB-14216
# @interface:  analyze(self,options)
# @author:     liuxiaoxuan 2018-01-29

from lib import testlib
from analyze.commlib import *
from pysequoiadb.error import (SDBBaseError, SDBEndOfCursor)

class TestAnalyzeCl14216(testlib.SdbTestBase):
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
      
   def test_analyze_cl_14216(self):
      # get groups
      groups = testlib.get_data_groups()
      src_groupname = groups[0]['GroupName']
      dest_groupname = groups[1]['GroupName']
   
      # create CLs
      cl_name1 = 'cll14216_1'
      cl_name2 = 'cll14216_2'
      cl_name3 = 'cll14216_3'
      
      self.cl = self.cs.create_collection(cl_name1, {'ReplSize': 0})
      options = {'Group': src_groupname, 'ShardingKey' : {'a' : 1}, 'ShardingType': 'hash', 'ReplSize': 0}
      self.hashcl = self.cs.create_collection(cl_name2, options)
      options = {'Group': src_groupname, 'ShardingKey' : {'a' : 1}, 'ShardingType': 'range', 'ReplSize': 0}
      self.rangecl = self.cs.create_collection(cl_name3, options)
   
      # insert datas
      insert_nums = 10000
      same_value = 0
      self.insert_diff_datas(self.cl, insert_nums)
      self.insert_same_datas(self.cl, insert_nums, same_value)
      self.insert_diff_datas(self.hashcl, insert_nums)
      self.insert_same_datas(self.hashcl, insert_nums, same_value)
      self.insert_diff_datas(self.rangecl, insert_nums)
      self.insert_same_datas(self.rangecl, insert_nums, same_value)
      
      # create indexs
      self.cl.create_index({'a':1}, 'a', False, False)
      self.hashcl.create_index({'a0':1}, 'a0', False, False)
      self.rangecl.create_index({'a0':1}, 'a0', False, False)
      
      # split cl
      self.hashcl.split_by_percent(src_groupname, dest_groupname, 50)
      self.rangecl.split_by_percent(src_groupname, dest_groupname, 50)
      
      # check explain before analyze
      cursor1 = self.cl.explain(condition = {'a' : 0}, options = {'Run' : True})
      cursor_hash1 = self.hashcl.explain(condition = {'a' : 0}, options = {'Run' : True})
      cursor_hash2 = self.hashcl.explain(condition = {'a0' : 0}, options = {'Run' : True})
      cursor_range1 = self.rangecl.explain(condition = {'a' : 0}, options = {'Run' : True})
      cursor_range2 = self.rangecl.explain(condition = {'a0' : 0}, options = {'Run' : True})
      
      act_explain1 = get_common_explain(cursor1)
      act_explainhash1 = get_split_explain(cursor_hash1)
      act_explainhash2 = get_split_explain(cursor_hash2)
      act_explainrange1 = get_split_explain(cursor_range1)
      act_explainrange2 = get_split_explain(cursor_range2)
      
      exp_explain1 = [{'ScanType': 'ixscan', 'IndexName': 'a', 'ReturnNum': insert_nums}]
      exp_explainhash1 = [{'GroupName': src_groupname, 'ScanType': 'ixscan', 'IndexName': '$shard', 'ReturnNum': insert_nums}]
      exp_explainhash2 = [{'GroupName': src_groupname, 'ScanType': 'ixscan', 'IndexName': 'a0', 'ReturnNum': insert_nums},
                          {'GroupName': dest_groupname, 'ScanType': 'ixscan', 'IndexName': 'a0', 'ReturnNum': 0}]
      exp_explainrange1 = [{'GroupName': src_groupname, 'ScanType': 'ixscan', 'IndexName': '$shard', 'ReturnNum': insert_nums}]
      exp_explainrange2 = [{'GroupName': src_groupname, 'ScanType': 'ixscan', 'IndexName': 'a0', 'ReturnNum': insert_nums},
                           {'GroupName': dest_groupname, 'ScanType': 'ixscan', 'IndexName': 'a0', 'ReturnNum': 0}]
      
      self.check_explain(exp_explain1, act_explain1)
      self.check_explain(exp_explainhash1, act_explainhash1)
      self.check_explain(exp_explainhash2, act_explainhash2)
      self.check_explain(exp_explainrange1, act_explainrange1)
      self.check_explain(exp_explainrange2, act_explainrange2)
      
      print('------check explain success before analyze------')
      
      # analyze cl1
      self.db.analyze(options = {'Collection': self.cs_name + "." + cl_name1})
      
      # check explain after analyze cl1
      cursor1 = self.cl.explain(condition = {'a' : 0}, options = {'Run' : True})
      cursor_hash1 = self.hashcl.explain(condition = {'a' : 0}, options = {'Run' : True})
      cursor_hash2 = self.hashcl.explain(condition = {'a0' : 0}, options = {'Run' : True})
      cursor_range1 = self.rangecl.explain(condition = {'a' : 0}, options = {'Run' : True})
      cursor_range2 = self.rangecl.explain(condition = {'a0' : 0}, options = {'Run' : True})
      
      act_explain1 = get_common_explain(cursor1)
      act_explainhash1 = get_split_explain(cursor_hash1)
      act_explainhash2 = get_split_explain(cursor_hash2)
      act_explainrange1 = get_split_explain(cursor_range1)
      act_explainrange2 = get_split_explain(cursor_range2)
      
      exp_explain1 = [{'ScanType': 'tbscan', 'IndexName': '', 'ReturnNum': insert_nums}]
      exp_explainhash1 = [{'GroupName': src_groupname, 'ScanType': 'ixscan', 'IndexName': '$shard', 'ReturnNum': insert_nums}]
      exp_explainhash2 = [{'GroupName': src_groupname, 'ScanType': 'ixscan', 'IndexName': 'a0', 'ReturnNum': insert_nums},
                          {'GroupName': dest_groupname, 'ScanType': 'ixscan', 'IndexName': 'a0', 'ReturnNum': 0}]
      exp_explainrange1 = [{'GroupName': src_groupname, 'ScanType': 'ixscan', 'IndexName': '$shard', 'ReturnNum': insert_nums}]
      exp_explainrange2 = [{'GroupName': src_groupname, 'ScanType': 'ixscan', 'IndexName': 'a0', 'ReturnNum': insert_nums},
                           {'GroupName': dest_groupname, 'ScanType': 'ixscan', 'IndexName': 'a0', 'ReturnNum': 0}]
      
      self.check_explain(exp_explain1, act_explain1)
      self.check_explain(exp_explainhash1, act_explainhash1)
      self.check_explain(exp_explainhash2, act_explainhash2)
      self.check_explain(exp_explainrange1, act_explainrange1)
      self.check_explain(exp_explainrange2, act_explainrange2)

      print('------check explain success after analyze cl1------')
      
      # analyze cl2
      self.db.analyze(options = {'Collection': self.cs_name + "." + cl_name2})
      
      # check explain after analyze cl2
      cursor1 = self.cl.explain(condition = {'a' : 0}, options = {'Run' : True})
      cursor_hash1 = self.hashcl.explain(condition = {'a' : 0}, options = {'Run' : True})
      cursor_hash2 = self.hashcl.explain(condition = {'a0' : 0}, options = {'Run' : True})
      cursor_range1 = self.rangecl.explain(condition = {'a' : 0}, options = {'Run' : True})
      cursor_range2 = self.rangecl.explain(condition = {'a0' : 0}, options = {'Run' : True})
      
      act_explain1 = get_common_explain(cursor1)
      act_explainhash1 = get_split_explain(cursor_hash1)
      act_explainhash2 = get_split_explain(cursor_hash2)
      act_explainrange1 = get_split_explain(cursor_range1)
      act_explainrange2 = get_split_explain(cursor_range2)
      
      exp_explain1 = [{'ScanType': 'tbscan', 'IndexName': '', 'ReturnNum': insert_nums}]
      exp_explainhash1 = [{'GroupName': src_groupname, 'ScanType': 'tbscan', 'IndexName': '', 'ReturnNum': insert_nums}]
      exp_explainhash2 = [{'GroupName': src_groupname, 'ScanType': 'tbscan', 'IndexName': '', 'ReturnNum': insert_nums},
                          {'GroupName': dest_groupname, 'ScanType': 'ixscan', 'IndexName': 'a0', 'ReturnNum': 0}]
      exp_explainrange1 = [{'GroupName': src_groupname, 'ScanType': 'ixscan', 'IndexName': '$shard', 'ReturnNum': insert_nums}]
      exp_explainrange2 = [{'GroupName': src_groupname, 'ScanType': 'ixscan', 'IndexName': 'a0', 'ReturnNum': insert_nums},
                           {'GroupName': dest_groupname, 'ScanType': 'ixscan', 'IndexName': 'a0', 'ReturnNum': 0}]
      
      self.check_explain(exp_explain1, act_explain1)
      self.check_explain(exp_explainhash1, act_explainhash1)
      self.check_explain(exp_explainhash2, act_explainhash2)
      self.check_explain(exp_explainrange1, act_explainrange1)
      self.check_explain(exp_explainrange2, act_explainrange2)

      print('------check explain success after analyze cl2------') 
      
      # analyze cl3
      self.db.analyze(options = {'Collection': self.cs_name + "." + cl_name3, 'Mode': 2})
      
      # check explain after analyze cl3
      cursor1 = self.cl.explain(condition = {'a' : 0}, options = {'Run' : True})
      cursor_hash1 = self.hashcl.explain(condition = {'a' : 0}, options = {'Run' : True})
      cursor_hash2 = self.hashcl.explain(condition = {'a0' : 0}, options = {'Run' : True})
      cursor_range1 = self.rangecl.explain(condition = {'a' : 0}, options = {'Run' : True})
      cursor_range2 = self.rangecl.explain(condition = {'a0' : 0}, options = {'Run' : True})
      
      act_explain1 = get_common_explain(cursor1)
      act_explainhash1 = get_split_explain(cursor_hash1)
      act_explainhash2 = get_split_explain(cursor_hash2)
      act_explainrange1 = get_split_explain(cursor_range1)
      act_explainrange2 = get_split_explain(cursor_range2)
      
      exp_explain1 = [{'ScanType': 'tbscan', 'IndexName': '', 'ReturnNum': insert_nums}]
      exp_explainhash1 = [{'GroupName': src_groupname, 'ScanType': 'tbscan', 'IndexName': '', 'ReturnNum': insert_nums}]
      exp_explainhash2 = [{'GroupName': src_groupname, 'ScanType': 'tbscan', 'IndexName': '', 'ReturnNum': insert_nums},
                          {'GroupName': dest_groupname, 'ScanType': 'ixscan', 'IndexName': 'a0', 'ReturnNum': 0}]
      exp_explainrange1 = [{'GroupName': src_groupname, 'ScanType': 'tbscan', 'IndexName': '', 'ReturnNum': insert_nums}]
      exp_explainrange2 = [{'GroupName': src_groupname, 'ScanType': 'tbscan', 'IndexName': '', 'ReturnNum': insert_nums},
                           {'GroupName': dest_groupname, 'ScanType': 'ixscan', 'IndexName': 'a0', 'ReturnNum': 0}]
      
      self.check_explain(exp_explain1, act_explain1)
      self.check_explain(exp_explainhash1, act_explainhash1)
      self.check_explain(exp_explainhash2, act_explainhash2)
      self.check_explain(exp_explainrange1, act_explainrange1)
      self.check_explain(exp_explainrange2, act_explainrange2)

      print('------check explain success after analyze cl3------') 

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
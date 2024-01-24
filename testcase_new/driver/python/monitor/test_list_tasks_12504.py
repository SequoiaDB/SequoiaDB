# @decription: test list tasks
# @testlink:   seqDB-12504
# @interface:  get_list(self,list_type,kwargs)
# @author:     liuxiaoxuan 2017-02-22

import unittest
import datetime
from pysequoiadb.error import (SDBBaseError, SDBEndOfCursor)
from lib import testlib

sdb_list_tasks = 10
class TestListTasks12504(testlib.SdbTestBase):
   def setUp(self):
      # skip standalone
      if testlib.is_standalone():
         self.skipTest('run mode is standalone')
      # skip one group
      if 2 > testlib.get_data_group_num():
         self.skipTest('less than two groups')   
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name) 

   def test_list_tasks_12504(self):
      # create cl
      groups = testlib.get_data_groups()
      src_groupname = groups[0]['GroupName']
      dest_groupname = groups[1]['GroupName']
      self.cl = self.cs.create_collection(self.cl_name, {'ShardingKey': {'a': 1}, 'Group': src_groupname})   
      
      # insert datas
      self.insert_datas()
      
      # split cl
      self.cl.split_async_by_percent(src_groupname, dest_groupname, 50.0)
      
      # list 10
      expect_result = {'Name': self.cs_name + "." + self.cl_name}
      act_result = self.get_list_tasks()
      # check list
      self.check_list(expect_result, act_result)
		
   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name) 

   def insert_datas(self):
      doc = []
      for i in range(0, 100000):
         doc.append({"a": i})
      try:
         self.cl.bulk_insert(0, doc)
      except SDBBaseError as e:   
         self.fail('insert fail: ' + str(e))
      
   def get_list_tasks(self):
      act_result = None
      try:
         cursor = self.db.get_list(sdb_list_tasks)
         # get result
         act_result = self.get_actual_result(cursor)
      except SDBBaseError as e:
         self.fail('get list tasks fail: ' + str(e))
      return act_result   
            
   def get_actual_result(self,cursor):
      item = {}
      while True:
         try:
            cl_full_name = self.cs_name + "." + self.cl_name
            rec = cursor.next()
            if cl_full_name == rec['Name']:
               item = {'Name': rec['Name']}
               cursor.close()
               break
         except SDBEndOfCursor:
            cursor.close()
            break
      return item
      
   def check_list(self, expect_result, act_result):
      self.assertEqual(expect_result['Name'], act_result['Name'])   
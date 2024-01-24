# @decription: split cl fail
# @testlink:   seqDB-13684
# @author:     liuxiaoxuan 2017-12-07

from lib import testlib
from pysequoiadb.error import (SDBBaseError, SDBTypeError)

class SplitException13684(testlib.SdbTestBase):
   def setUp(self):
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      if testlib.is_standalone():
         self.skipTest('skip standalone')  
         
      if 2 > testlib.get_data_group_num():
         self.skipTest('group less than 2')
         
   def test_split_13684(self):
      # create shard cl
      groups = testlib.get_data_groups()
      src_group = groups[0]['GroupName']
      dest_group = groups[1]['GroupName']
      
      cl_option = {'ShardingKey' : {'a' : 1}, 'Group' : src_group}
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name, cl_option)
     
      # check split cl
      percent_int = 50
      self.check_error_split_cl(src_group, dest_group, percent_int)
      
   def check_error_split_cl(self, src, dest, percent_int):
      try:
         self.cl.split_async_by_percent(src, dest, percent_int)
         self.fail("NEED TYPE ERROR")
      except SDBTypeError as e:
         self.assertEqual(e.code, -6)
         self.assertEqual(e.detail, "percent must be an instance of float")   

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)          

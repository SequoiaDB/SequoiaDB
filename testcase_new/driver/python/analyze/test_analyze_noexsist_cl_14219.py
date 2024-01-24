# @decription: analyze cl no exist
# @testlink:   seqDB-14219
# @interface:  analyze(self,options)
# @author:     liuxiaoxuan 2018-01-30

from lib import testlib
from analyze.commlib import *
from pysequoiadb.error import (SDBBaseError, SDBEndOfCursor)

class TestAnalyzeInvalidCL14219(testlib.SdbTestBase):
   def setUp(self):
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      # create cs
      self.cs = self.db.create_collection_space(self.cs_name)
      
   def test_analyze_invalid_cl_14219(self):
      noexist_clname = 'NONE'
      sys_clname = 'SYSSTAT.SYSCOLLECTIONSTAT'
      sys_indexname = 'SYSSTAT.SYSINDEXSTAT'
      
      self.check_analyze_invalid_result({'Collection' : self.cs_name + "." + noexist_clname}, -23)
      self.check_analyze_invalid_result({'Collection' : sys_clname}, -6)
      self.check_analyze_invalid_result({'Collection' : sys_indexname}, -6)

   def tearDown(self):
      # drop cs
      if self.should_clean_env():
         testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
         
   def check_analyze_invalid_result(self, opt, errcode):
      try:
         self.db.analyze(options = opt)
         self.fail('Need Analyze Fail')
      except SDBBaseError as e:  
         if errcode != e.code:      
            self.fail('insert fail: ' + str(e))
        
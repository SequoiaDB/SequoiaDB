# @decription: exec invalid sql
# @testlink:   seqDB-13685
# @author:     liuxiaoxuan 2017-12-07

from lib import testlib
from pysequoiadb.error import (SDBBaseError, SDBError)

class sqlException13685(testlib.SdbTestBase):
   def setUp(self):
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)

   def test_sql_13685(self):
      # check exec invalid sql
      sql = "select from " + self.cs_name + "." + self.cl_name 
      self.check_error_exec_sql(sql)
      
   def check_error_exec_sql(self, sql):
      try:
         self.db.exec_sql(sql)     
         self.fail("NEED SDB ERROR")         
      except SDBError as e:
         self.assertEqual(e.code, -195)
         self.assertEqual(e.detail, "Failed to execute sql: " + sql)   

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)          

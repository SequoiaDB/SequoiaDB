# @decription: insert invalid records, find end of cursor, update/delete without $id
# @testlink:   seqDB-13679/seqDB-13680/seqDB-13691
# @author:     liuxiaoxuan 2017-12-07

from lib import testlib
from pysequoiadb.error import (SDBBaseError, SDBError, SDBEndOfCursor)

class CurdException13679(testlib.SdbTestBase):
   def setUp(self):
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      # create cl with non $id
      cl_option = {'AutoIndexId' : False}
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name, cl_option)

   def test_curd_13679(self):
      # create unique index
      is_unique = True
      self.cl.create_index({'a' : 1}, 'a', is_unique)
     
      # insert data
      same_record = {"a" : 1}
      self.cl.insert(same_record)
         
      # check insert same data
      self.check_error_insert_datas(same_record)
      
      # check query after end of cursor
      self.check_error_find_end_cursor()
      
      # check update/delete
      rule = {'$set' : {'a' : -999}}
      matcher = {'a' : {'$gt' : 0}}
      self.check_error_update(rule, matcher)
      self.check_error_delete(matcher)
      
   def check_error_insert_datas(self, record):
      try:
         self.cl.insert(record)
         self.fail("NEED SDB ERROR")
      except SDBError as e:
         self.assertEqual(e.code, -38)
         self.assertEqual(e.detail, "Failed to insert record") 
            
   def check_error_find_end_cursor(self):
      cl_count = self.cl.get_count()
      cursor = self.cl.query()
      for i in range(cl_count):
         cursor.next()     
         
      try:
         cursor.next();
         self.fail("NEED END OF CURSOR ERROR")
      except SDBEndOfCursor as e:
         self.assertEqual(e.code, -29)
         self.assertEqual(e.detail, "end of cursor") 
         
   def check_error_update(self, rule, matcher):
      try:
         self.cl.update(rule, condition = matcher)
         self.fail("NEED SDB ERROR")
      except SDBError as e:
         self.assertEqual(e.code, -279)
         self.assertEqual(e.detail, "Failed to update")   
         
   def check_error_delete(self, matcher):
      try:
         self.cl.delete(condition = matcher)
         self.fail("NEED SDB ERROR")
      except SDBError as e:
         self.assertEqual(e.code, -279)
         self.assertEqual(e.detail, "Failed to delete")        

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)

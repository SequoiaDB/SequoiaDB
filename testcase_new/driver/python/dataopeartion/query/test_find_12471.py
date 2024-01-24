# @decription: find records with query/get_count/close/current/next/close_all_cursors
# @testlink:   seqDB-12471
# @interface:  query(self,kwargs)
#              get_count(self,condition)
#              close(self)
#              current(self,ordered)
#              close_all_cursors(self)
# @author:     liuxiaoxuan 2017-8-29

from lib import testlib
from pysequoiadb.error import (SDBBaseError, SDBEndOfCursor)

insert_nums = 100
class TestFind12471(testlib.SdbTestBase):
   def setUp(self):
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)
      self.insert_datas()

   def test_find_12471(self):
      # query all records
      expectResult = []
      for i in range(0, insert_nums):
         expectResult.append({"_id": i, "a": "test" + str(i)})
      self.query_datas(expectResult)

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)

   def insert_datas(self):
      doc = []
      for i in range(0, insert_nums):
         doc.append({"_id": i, "a": "test" + str(i)})
      try:
         flags = 0
         self.cl.bulk_insert(flags, doc)
      except SDBBaseError as e:
         self.fail('insert fail: ' + str(e))

   def get_count(self, expect_count, cond):
      try:
         if cond == None:
            act_count = self.cl.get_count()
         else:
            act_count = self.cl.get_count(condition = cond)
         self.assertEqual(expect_count, act_count)
      except SDBBaseError as e:
         self.fail('get count fail: ' + str(e))

   def query_datas(self, expectResult):
	   # find all records
      cursor_all = self.cl.query()
      actResult = testlib.get_all_records(cursor_all)
      self.assertListEqualUnordered(expectResult, actResult)
		# check the cursor when closed
      self.check_close_cursor_next(cursor_all)
      self.check_close_cursor_current(cursor_all)
		
		# SEQUOIADBMAINSTREAM-2784, continue checking cursor when end of cursor
      cursor_end = self.cl.query()
      self.goto_end_of_cursor(cursor_end)
      self.check_close_cursor_next(cursor_end)
      self.check_close_cursor_current(cursor_end)

		# check counts
      cond1 = {"_id": {"$et": 1}}
      cursor1 = self.cl.query(condition = cond1)
      expect_count1 = 1
      self.get_count(expect_count1, cond1)

      cond2 = {"_id": {"$gt": 90}}
      cursor2 = self.cl.query(condition = cond2)
      expect_count2 = 9
      self.get_count(expect_count2, cond2)

		# check the cursors when all closed
      self.db.close_all_cursors()
      self.check_close_cursor_next(cursor1)
      self.check_close_cursor_current(cursor1)
      self.check_close_cursor_next(cursor2)
      self.check_close_cursor_current(cursor2)

   def goto_end_of_cursor(self,cursor):
      while(True):
         try:
            cursor.next()
         except SDBEndOfCursor:     		
            break;  
      				
   def check_close_cursor_next(self, cursor):
      try:
         cursor.next()
         self.fail("need next cursor fail")
      except SDBBaseError as e:
         self.assertEqual(-31, e.code, 'the error is not -31: ' + str(e.code))

   def check_close_cursor_current(self, cursor):
      try:
         cursor.current()
         self.fail("need current cursor fail")
      except SDBBaseError as e:
         self.assertEqual(-31, e.code, 'the error is not -31: ' + str(e.code))
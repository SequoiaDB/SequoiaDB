# @decription: query metas with option
# @testlink:   seqDB-12479 
# @interface:  get_query_meta(self,kwargs)
# @author:     liuxiaoxuan 2017-10-18

from bson.py3compat import (long_type)
from lib import testlib
from pysequoiadb.error import (SDBBaseError)


class TestFind12479(testlib.SdbTestBase):
   def setUp(self):
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)
		#create index
      index = {'a' : 1}
      index_name = 'a'
      self.create_index(index, index_name)
		#insert data
      self.insert_datas()

   def test_find_12479(self):
	   # query with index(not specify index_name)
      expectResult1 = {'ScanType': 'ixscan' , 'IndexName': 'a'}
      condition1 = {"a": {'$gt': 10,'$lt': 20}}
      skip1 = long_type(1)
      retrn1 = long_type(5)
      self.query_meta_with_kwargs(expectResult1, 
		                            condition = condition1, order_by = {'a' : 1}, \
		                            num_to_skip = skip1, num_to_return = retrn1)

      # query with tbscan
      expectResult2 = {'ScanType': 'tbscan', 'IndexName': None}		
      condition2 = {"a": {'$gte': 90}}
      hint2 = {'': None}
      skip2 = long_type(5)
      retrn2 = long_type(6)
      self.query_meta_with_kwargs(expectResult2, 
		                            condition = condition2, order_by = {'a' : 1}, \
											 hint = hint2, \
		                            num_to_skip = skip2, num_to_return = retrn2)		   

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)

   def insert_datas(self):
      insert_nums = 100
      for i in range(1, insert_nums):
         try:
            self.cl.insert({"a": i, "b": "test" + str(i)})
         except SDBError as e:
            self.fail('insert fail: ' + str(e))
			
   def create_index(self, index, index_name):
      try:
         isUnique = False
         isEnforced = False
         self.cl.create_index(index, index_name, isUnique, isEnforced)
         print ('create index success')

      except SDBBaseError as e:
         self.fail('create index fail: ' + str(e))

   def query_meta_with_kwargs(self, expectResult, **kwargs):
      try:		
         rec = self.cl.get_query_meta(**kwargs)
			
         actResult = rec.next()
         self.check_query_result(actResult, expectResult)
      except SDBBaseError as e:
         self.fail('query meta fail: ' + str(e))
			
   def check_query_result(self, actResult, expectResult):
      if not(expectResult['IndexName'] is None):
         actIdxName = actResult['IndexName']
         eptIdxName = expectResult['IndexName']
         self.assertEqual(actIdxName, eptIdxName) 		
      self.assertEqual(actResult['ScanType'], expectResult['ScanType']) 
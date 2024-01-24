# @decription: find records with option
# @testlink:   seqDB-12470
# @interface:  query(self,kwargs)
#              get_count(self,condition)
# @author:     liuxiaoxuan 2017-8-29

from bson.py3compat import (long_type)
from pysequoiadb import collection
from lib import testlib
from pysequoiadb.error import (SDBBaseError, SDBEndOfCursor)

class TestFind12470(testlib.SdbTestBase):
   def setUp(self):
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)
      self.insert_datas()

   def test_find_12470(self):
      # condition:$gt,$lt, selection:$include
      condition1 = {"a": {'$gt': 0, '$lt': 100}}
      selected1 = {"a": {"$include": 1}}
      expectCount1 = 3
      expectResult1 = [{"a": 2}, {"a": [10, 20, 30]}]
      self.query_with_kwargs(condition1, selected1, expectResult1)
      self.get_count(condition1, expectCount1)

      # condition:$mod, selection:$include
      condition2 = {"_id": {'$mod': [2, 1]}}
      selected2 = {"a": {"$include": 1}}
      expectCount2 = 7
      expectResult2 = [{"a": 101.1}, \
                      {"a": {"b": 1}}, \
                    {"a": [{"b": 'b'}, {"c": 'c'}]}, \
                    {"a": {"$regex": "^a", "$options": "i"}}, \
                    {"a": None}, \
                    {"a": {"$date": "2017-09-01"}}]
      self.query_with_kwargs(condition2, selected2, expectResult2)
      self.get_count(condition2, expectCount2)

      # condition:$isnull, selection:$elemMatch
      condition3 = {"_id": {'$isnull': 0}}
      selected3 = {"a": {"$elemMatch": {"b": 'b'}}}
      expectCount3 = 13
      expectResult3 = [{"_id": 2}, \
                       {"_id": 3}, \
                       {"_id": 4}, \
                       {"_id": 5}, \
                       {"_id": 6, "a": []}, \
                       {"_id": 7, "a": [{"b": 'b'}]}, \
                       {"_id": 8, "a": [{"b": 'b'}]}, \
                       {"_id": 9}, \
                       {"_id": 10}, \
                       {"_id": 11}]
      self.query_with_kwargs(condition3, selected3, expectResult3)
      self.get_count(condition3, expectCount3)

      # condition:$and, selection:$include
      condition4 = {"$and": [{'_id': {'$gt': 1}}, {'a': {'$lt': 100}}]}
      selected4 = {"a": {"$include": 1}}
      expectCount4 = 2
      expectResult4 = [{"a": [10, 20, 30]}]
      self.query_with_kwargs(condition4, selected4, expectResult4)
      self.get_count(condition4, expectCount4)

      # condition:$field, selection:$include
      condition5 = {"_id": {"$field": "a"}}
      selected5 = {"a": {"$include": 1}}
      expectCount5 = 2
      expectResult5 = [{"a": 2}]
      self.query_with_kwargs(condition5, selected5, expectResult5)
      self.get_count(condition5, expectCount5)
	
	   # condition:$expand
      condition6 = {"a": {"$expand": 1}}
      selected6 = {}
      expectCount6 = 17
      expectResult6 = [{"_id": 2, "a": 2}, \
                       {"_id": 3, "a": 101.1}, \
                       {"_id": 4, "a": 'abc'}, \
                       {"_id": 5, "a": {"b": 1}}, \
                       {"_id": 6, "a": 10}, \
                       {"_id": 6, "a": 20}, \
                       {"_id": 6, "a": 30}, \
                       {"_id": 7, "a": {"b": 'b'}}, \
                       {"_id": 7, "a": {"c": 'c'}}, \
                       {"_id": 8, "a": {"b": 'b'}}]
      self.query_with_kwargs(condition6, selected6, expectResult6)
      self.get_count(condition6, expectCount6)

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)

   def query_with_kwargs(self, cond, selection, expectResult):
      try:
         skip = long_type(1)
         retrn = long_type(10)
         cursor = self.cl.query(condition = cond, \
                                selector = selection, \
                                order_by = {"_id": 1}, \
                                hint = {"": "$id"}, \
                                num_to_skip = skip, \
                                num_to_return = retrn, \
                                flags = collection.QUERY_FLG_FORCE_HINT)

         actResult = testlib.get_all_records(cursor)
         self.assertListEqualUnordered(expectResult, actResult)
      except SDBBaseError as e:
         self.fail('check query fail: ' + str(e))

   def insert_datas(self):
      flags = 0
      doc = [{"_id": 1, "a": 1}, \
             {"_id": 2, "a": 2}, \
             {"_id": 3, "a": 101.1}, \
             {"_id": 4, "a": 'abc'}, \
             {"_id": 5, "a": {"b": 1}}, \
             {"_id": 6, "a": [10, 20, 30]}, \
             {"_id": 7, "a": [{"b": 'b'}, {"c": 'c'}]}, \
             {"_id": 8, "a": [{"b": 'b'}, {"c": 'newc'}]}, \
             {"_id": 9, "a": {"$regex": "^a", "$options": "i"}}, \
             {"_id": 10, "a": {"$regex": "^b", "$options": "i"}}, \
             {"_id": 11, "a": None}, \
             {"_id": 12, "a": True}, \
             {"_id": 13, "a": {"$date": "2017-09-01"}}]
      try:
         self.cl.bulk_insert(flags, doc)
      except SDBBaseError as e:
         self.fail('insert fail: ' + str(e))


   def get_count(self, condition, expectCount):
      try:
         actCount = self.cl.get_count(condition)
         self.assertEqual(expectCount, actCount)
      except SDBBaseError as e:
         self.fail('get count fail: ' + str(e))

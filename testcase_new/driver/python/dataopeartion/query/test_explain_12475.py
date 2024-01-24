# @decription: test explain
# @testlink:   seqDB-12475
# @interface:  explain(self,kwargs)
# @author:     liuxiaoxuan 2017-8-30

from bson.py3compat import (PY3,long_type)
from pysequoiadb.error import (SDBTypeError, SDBBaseError, SDBEndOfCursor,SDBError)
from lib import testlib

class TestExplain12475(testlib.SdbTestBase):
   def setUp(self):
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)
      self.insert_datas()
	
   def test_explain_12475(self):
      aIndex = {'a': 1}
      aIdxName = 'a'
      self.create_index(aIndex, aIdxName)
      self.check_index(aIdxName)

      expScanType = 'ixscan'
      expIdxName = aIdxName
      # condition:$gt,$lt, selection:$include
      condition1 = {"a": {'$gt': 0, '$lt': 100}}
      selected1 = {"a": {"$include": 1}}
      expQuery1 = {"$and": [{"a": {"$gt": 0}}, {"a": {"$lt": 100}}]}
      expectResult1 = {"expScanType": expScanType, "expIdxName": expIdxName, "Query": expQuery1}
      self.get_explain(expectResult1, condition1, selected1, aIdxName)

      # condition:$mod, selection:$include
      condition2 = {"_id": {'$mod': [2, 1]}}
      selected2 = {"a": {"$include": 1}}
      expQuery2 = {"$and": [{"_id": {"$mod": [2, 1]}}]}
      expectResult2 = {"expScanType": expScanType, "expIdxName": expIdxName, "Query": expQuery2}
      self.get_explain(expectResult2, condition2, selected2, aIdxName)

      # condition:$isnull, selection:$elemMatch
      condition3 = {"_id": {'$isnull': 0}}
      selected3 = {"a": {"$elemMatch": {"b": 'b'}}}
      expQuery3 = {"$and": [{"_id": {"$isnull": 0}}]}
      expectResult3 = {"expScanType": expScanType, "expIdxName": expIdxName, "Query": expQuery3}
      self.get_explain(expectResult3, condition3, selected3, aIdxName)

      # condition:$and, selection:$include
      condition4 = {"$and": [{'_id': {'$gt': 1}}, {'a': {'$lt': 100}}]}
      selected4 = {"a": {"$include": 1}}
      expQuery4 = {"$and": [{"_id": {"$gt": 1}}, {"a": {"$lt": 100}}]}
      expectResult4 = {"expScanType": expScanType, "expIdxName": expIdxName, "Query": expQuery4}
      self.get_explain(expectResult4, condition4, selected4, aIdxName)

      # condition:$field, selection:$include
      condition5 = {"_id": {"$field": "a"}}
      selected5 = {"a": {"$include": 1}}
      expQuery5 = {"$and": [{"_id": {"$et": {"$field": "a"}}}]}
      expectResult5 = {"expScanType": expScanType, "expIdxName": expIdxName, "Query": expQuery5}
      self.get_explain(expectResult5, condition5, selected5, aIdxName)

      # condition:$expand
      condition6 = {"a": {"$expand": 1}}
      selected6 = {}
      expQuery6 = {"$and": [{"a": {"$expand": 1}}]}
      expectResult6 = {"expScanType": expScanType, "expIdxName": expIdxName, "Query": expQuery6}
      self.get_explain(expectResult6, condition6, selected6, aIdxName)

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)		
			
   def insert_datas(self):
      flags = 0
      doc = [{"_id": 1, "a": 1}, \
             {"_id": 2, "a": 2}, \
             {"_id": 3, "a": 101.1}, \
             {"_id": 4, "a": 'abc'}, \
                {"_id": 5, "a": [10, 20, 30]}, \
                {"_id": 6, "a": [100, 200, 300]}, \
                {"_id": 7, "a": [{"b": 'b'}, {"c": 'c'}]}, \
                {"_id": 8, "a": [{"b": 'b'}, {"c": 'newc'}]}, \
                {"_id": 9, "a": {"$regex": "^a", "$options": "i"}}, \
                {"_id": 10, "a": {"$regex": "^b", "$options": "i"}}, \
                {"_id": 11, "a": None}, \
                {"_id": 12, "a": True}, \
                {"_id": 13, "a": {"$date": "2017-09-01"}}]
      try:
         self.cl.bulk_insert(flags, doc)
      except SDBError as e:
         self.fail('insert fail: ' + str(e))

   def create_index(self,index,index_name):
      try:
         self.cl.create_index(index, index_name, False, False, 64)
      except SDBBaseError as e:
         self.fail('create index fail: ' + str(e))

   def check_index(self,index_name):
      try:
         cursor = self.cl.get_indexes(index_name);  
         rec = cursor.next()
         act_idx_name = rec['IndexDef']['name']
         exp_idx_name = index_name
         self.assertEqual(exp_idx_name, act_idx_name,'index name is not exist')
      except SDBBaseError as e:
         self.fail('check index fail: ' + str(e))
      
   def get_explain(self,expectExplainRec,cond,selection,index_name):
      try:
         cursor = self.cl.explain(condition = cond,\
                                  selected = selection,\
                                  order_by = {"_id":1},\
                                  hint = {"":index_name},\
                                  num_to_skip = long_type(1),\
                                  num_to_return = long_type(5),\
                                  flags = 1)
         rec = cursor.next()  
         expScanType = expectExplainRec['expScanType']
         expIdxName = expectExplainRec['expIdxName']
         expQuery = expectExplainRec['Query']['$and']
         actScanType = rec['ScanType']
         actIdxName = rec['IndexName']
         actQuery= rec['Query']['$and']
         self.assertEqual( expScanType,actScanType)
         self.assertEqual( expIdxName,actIdxName)
         self.assertListEqualUnordered(expQuery, actQuery)
      except SDBBaseError as e:
         self.fail('check explain fail: ' + str(e))

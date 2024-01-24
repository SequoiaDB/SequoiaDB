# @decription: create/drop/query common index
# @testlink:   seqDB-12476
# @interface:  create_index(self,index_def,idx_name,is_unique,is_enforced,buffer_size)
#              drop_index(self,idx_name)
#              get_index_info(self,idx_name)
#              is_index_exist(self,idx_name)
# @author:     liuxiaoxuan 2017-8-30

from lib import testlib
from pysequoiadb.error import (SDBBaseError, SDBEndOfCursor, SDBError)

insert_nums = 100


class TestIndex12476(testlib.SdbTestBase):
   def setUp(self):
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)
      self.insert_datas()

   def test_index_12476(self):
      try:
         aIndex = {'a': 1}
         aIdxName = 'a'
         isOption = True
         self.create_index(aIndex, aIdxName, isOption)

         condition = {"a": {'$gt': 1, '$lt': 11}}
         expectResult = []
         for i in range(2, 11):
            expectResult.append({"_id": i, "a": i, "b": "test" + str(i)})
         self.query_datas(expectResult, condition, aIdxName)

         expScanType = 'ixscan'
         expIdxName = aIdxName
         expQuery = {"$and": [{"a": {"$gt": 1}}, {"a": {"$lt": 11}}]}
         expectExplainRec = {"expScanType": expScanType, "expIdxName": expIdxName, "Query": expQuery}
         self.check_explain(expectExplainRec, condition)

         expectIdxResult = {"expIdName": expIdxName, "expKey": aIndex, "expUnique": isOption, "expEnforced": isOption}
         self.check_indexes(expectIdxResult, aIdxName)
         
         # check is_index_exist
         self.assertTrue(self.cl.is_index_exist(aIdxName))
         self.assertFalse(self.cl.is_index_exist('b'))

         self.drop_index(aIdxName)
         self.assertFalse(self.cl.is_index_exist(aIdxName))
         expScanType = 'tbscan'
         expIdxName = ''
         expectExplainRec = {"expScanType": expScanType, "expIdxName": expIdxName, "Query": expQuery}
         self.query_datas(expectResult, condition, None)
         self.check_explain(expectExplainRec, condition)

      except SDBBaseError as e:
         self.fail("test idIndex failed:" + str(e))

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)

   def insert_datas(self):
      for i in range(1, insert_nums):
         try:
            self.cl.insert({"_id": i, "a": i, "b": "test" + str(i)})
         except SDBError as e:
            self.fail('insert fail: ' + str(e))

   def create_index(self, index, index_name, isOption):
      try:
         if isOption:
            isUnique = True
            isEnforced = True
            buffer_size = 128
            self.cl.create_index(index, index_name, isUnique, isEnforced, buffer_size)
         else:
            pass
         print ('create index success')

      except SDBBaseError as e:
         self.fail('create index fail: ' + str(e))

   def drop_index(self, index_name):
      try:
         self.cl.drop_index(index_name)
         print('drop index success')
      except SDBBaseError as e:
         self.fail('drop index fail: ' + str(e))

   def check_indexes(self, expectResult, index_name):
      try:
         rec = self.cl.get_index_info(index_name)
         index_name = rec['IndexDef']['name']
         key = rec['IndexDef']['key']
         isUnique = rec['IndexDef']['unique']
         isEnforced = rec['IndexDef']['enforced']
         self.assertEqual(index_name, expectResult['expIdName'])
         self.assertEqual(key, expectResult['expKey'])
         self.assertEqual(isUnique, expectResult['expUnique'])
         self.assertEqual(isEnforced, expectResult['expEnforced'])
      except SDBBaseError as e:
         self.fail('check index fail: ' + str(e))

   def query_datas(self, expectResult, cond, index_name):
      try:
         cursor = self.cl.query(condition=cond, \
                                order_by={"_id": 1}, \
                                hint={"": index_name}, \
                                flags=1)
         actResult = []
         while True:
            try:
               rec = cursor.next()
               actResult.append(rec)
            except SDBEndOfCursor:
               break
         self.assertListEqualUnordered(expectResult, actResult)
      except SDBBaseError as e:
         self.fail('query fail: ' + str(e))

   def check_explain(self, expectExplainRec, cond):
      try:
         cursor = self.cl.explain(condition=cond, \
                                  order_by={"b": 1}, \
                                  flags=1)
         rec = cursor.next()
         expScanType = expectExplainRec['expScanType']
         expIdxName = expectExplainRec['expIdxName']
         expQuery = expectExplainRec['Query']['$and']
         actScanType = rec['ScanType']
         actIndexName = rec['IndexName']
         actQuery = rec['Query']['$and']
         self.assertEqual(expScanType, actScanType)
         self.assertEqual(expIdxName, actIndexName)
         self.assertListEqualUnordered(expQuery, actQuery)
      except SDBBaseError as e:
         self.fail('check explain fail: ' + str(e))

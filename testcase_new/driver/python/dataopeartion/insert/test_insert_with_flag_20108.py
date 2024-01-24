# @decription: insert record with flag
# @testlink:   seqDB-20108
# @interface:  insert_with_flag(self, record, flag=INSERT_FLG_DEFAULT)
# @author:     yinzhen 2019-10-28

from dataopeartion.insert.commlib import *
from lib import testlib
from pysequoiadb.error import (SDBBaseError)
from pysequoiadb.collection import (INSERT_FLG_DEFAULT, INSERT_FLG_CONTONDUP, INSERT_FLG_RETURN_OID, INSERT_FLG_REPLACEONDUP, INSERT_FLG_RETURNNUM, INSERT_FLG_CONTONDUP_ID, INSERT_FLG_REPLACEONDUP_ID)
from bson import ObjectId

class TestInsertWithFlag20108(testlib.SdbTestBase):
   def setUp(self):

      # create cs and cl
      self.cs_name = "cs_20108"
      self.cl_name = "cl_20108"
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)

   def test_insert_with_flag_20108(self):

      # insert data with default flag
      record = {"a": 1, "b": 1}
      ret_value = self.cl.insert_with_flag(record)

      # query data and check
      check_Result(self.cl, {"a": 1}, [record], False)
      self.assertEqual({'DuplicatedNum': 0, 'InsertedNum': 1, 'ModifiedNum': 0}, ret_value)

      # insert data with INSERT_FLG_DEFAULT
      self.cl.create_index({"a":1}, "idx", True)
      record2 = {"a": 1, "b": 2}
      try:
         self.cl.insert_with_flag(record2, INSERT_FLG_DEFAULT)
         self.fail("need throw error")
      except SDBBaseError as e:
         self.assertEqual(-38, e.code)

      # query data and check
      check_Result(self.cl, {"a": 1}, [record], False)

      # insert data with INSERT_FLG_CONTONDUP
      self.cl.insert_with_flag(record2, INSERT_FLG_CONTONDUP)

      # query data and check
      check_Result(self.cl, {"a": 1}, [record], False)

      # insert data with INSERT_FLG_RETURN_OID
      record2 = {"a": 2, "b": 2}
      ret_value = self.cl.insert_with_flag(record2, INSERT_FLG_RETURN_OID)

      # query data and check
      self.assertTrue(isinstance(ret_value["_id"], ObjectId))
      record2 = {"_id":ret_value["_id"], "a":2, "b":2}
      check_Result(self.cl, {"a":2}, [record2], True)

      # insert data with INSERT_FLG_REPLACEONDUP
      record3 = {"a": 1, "b": 2}
      ret_value = self.cl.insert_with_flag(record3, INSERT_FLG_REPLACEONDUP)

      # query data and check
      check_Result(self.cl, {"a":1}, [record3], False)
      self.assertEqual({'DuplicatedNum': 1, 'InsertedNum': 0, 'ModifiedNum': 1}, ret_value)

      # insert data with INSERT_FLG_RETURNNUM
      record4 = {"a": 3, "b": 3}
      ret_value = self.cl.insert_with_flag(record4, INSERT_FLG_RETURNNUM)

      # query data and check
      check_Result(self.cl, {"a":3}, [record4], False)
      self.assertEqual({"DuplicatedNum": 0, "InsertedNum": 1, 'ModifiedNum': 0}, ret_value)

      # insert data with INSERT_FLG_CONTONDUP_ID
      idRecord1 = {"_id": 1, "b": 1}
      self.cl.insert_with_flag(idRecord1)

      idRecord2 = {"_id":1, "b": 2}
      ret_value = self.cl.insert_with_flag(idRecord2, INSERT_FLG_CONTONDUP_ID)

      # query data and check
      check_Result(self.cl, {"_id": 1}, [idRecord1], True)
      self.assertEqual({'DuplicatedNum': 1, 'InsertedNum': 0, 'ModifiedNum': 0}, ret_value)

      # insert data with INSERT_FLG_REPLACEONDUP_ID
      idRecord3 = {"_id":1, "b":3}
      ret_value = self.cl.insert_with_flag(idRecord3, INSERT_FLG_REPLACEONDUP_ID)

      # query data and check
      check_Result(self.cl, {"_id": 1}, [idRecord3], True)
      self.assertEqual({'DuplicatedNum': 1, 'InsertedNum': 0, 'ModifiedNum': 1}, ret_value)

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)

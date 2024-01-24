# @decription: insert record with flag
# @testlink:   seqDB-20111
# @interface:  insert_with_flag(self, record, flag=INSERT_FLG_DEFAULT)
# @author:     yinzhen 2019-10-28

from dataopeartion.insert.commlib import *
from lib import testlib
from pysequoiadb.collection import (INSERT_FLG_RETURN_OID, INSERT_FLG_RETURNNUM)
from bson import ObjectId

class TestBulkInsert20111(testlib.SdbTestBase):
   def setUp(self):

      # create cs and cl
      self.cs_name = "cs_20111"
      self.cl_name = "cl_20111"
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)
      
   def test_bulk_insert20111(self):

      # insert data with INSERT_FLG_RETURN_OID
      records = [{"a": 1, "b": 1}, {"a": 2, "b": 2}, {"a": 3, "b": 3}]
      ret_value = self.cl.bulk_insert(INSERT_FLG_RETURN_OID, records)

      # query data and check
      ret_oids = ret_value["_id"]
      count = 0
      for oid in ret_oids:
         count += 1
         self.assertTrue(isinstance(oid, ObjectId))
         record = {"_id":oid, "a": count, "b": count}
         check_Result(self.cl, {"a":count}, [record], True)

      ret_value = self.cl.bulk_insert(INSERT_FLG_RETURNNUM, records)
      self.assertEqual({"DuplicatedNum": 0, "InsertedNum": len(records), 'ModifiedNum': 0}, ret_value)
      self.assertEqual(self.cl.get_count(), 2*len(records))

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)

# @decription: test reset snapshot
# @testlink:   seqDB-14451
# @interface:  reset_snapshot(self,condition)
# @author:     linsuqiang 2018-02-08

import unittest
import datetime
from pysequoiadb.error import (SDBTypeError, SDBBaseError, SDBEndOfCursor, SDBError)
import time
from lib import testlib

class TestResetSnapshot14451(testlib.SdbTestBase):
   def setUp(self):
      if testlib.is_standalone():
         self.skipTest('current environment is standalone')
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)

      data_group = testlib.get_data_groups()[0]
      self.group_name = data_group["GroupName"]
      options = {"Group": self.group_name}
      cl = self.cs.create_collection(self.cl_name, options)
      self.insert_data(cl)

   def test_reset_snapshot_14451(self):
      group_obj = self.db.get_replica_group_by_name(self.group_name)
      dataDB = group_obj.get_master().connect()

      self.create_statis_info(dataDB)
      self.db.reset_snapshot({ "Type": "sessions" })
      isClean = self.isSessionSnapClean(dataDB)
      self.assertTrue(isClean, "sessions snapshot hasn't been reset")
      isClean = self.isDatabaseSnapClean(dataDB)
      self.assertFalse(isClean, "database snapshot shouldn't been reset")

      dataDB.disconnect()
		
   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)

   def insert_data(self, cl):
      insert_num = 100
      flag = 0
      doc = []
      for i in range(0, insert_num):
         doc.append({ "a": 1 })
      try:
         cl.bulk_insert(flag, doc)
      except SDBError as e:
         self.fail('insert fail: ' + str(e))

   def create_statis_info(self, dataDB):
      cs = dataDB.get_collection_space(self.cs_name)
      cl = cs.get_collection(self.cl_name)
      cur = cl.query()
      while True:
         try:
            record = cur.next()
         except SDBEndOfCursor:
            break
         except SDBBaseError:
            raise
      cur.close()

   def isSessionSnapClean(self, dataDB):
      # 2: all sessions's snapshot
      cond = { "Type": "Agent" }
      cur = dataDB.get_snapshot(2, condition = cond)
      rec = cur.next()
      totalRead = rec["TotalRead"]
      cur.close()
      return (totalRead == 0)
      
   def isDatabaseSnapClean(self, dataDB):
      # 6: database's snapshot
      cur = dataDB.get_snapshot(6)
      rec = cur.next()
      totalRead = rec["TotalRead"]
      cur.close()
      return (totalRead == 0)

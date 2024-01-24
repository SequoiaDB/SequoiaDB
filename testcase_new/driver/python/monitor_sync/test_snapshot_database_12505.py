# @decription: test snapshot database
# @testlink:   seqDB-12505
# @interface:  get_snapshot(self,snap_type,kwargs)
# @author:     liuxiaoxuan 2018-02-22

import unittest
import datetime
from pysequoiadb.error import SDBBaseError, SDBEndOfCursor
from lib import testlib

sdb_snap_database = 6
class TestSnapshotDataBase12505(testlib.SdbTestBase):
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

   def test_snapshot_database_12505(self):
      expect_result = ["TotalDataRead", "TotalIndexRead"]
      # snapshot 6
      cursor = self.db.get_snapshot(sdb_snap_database)
      act_result = testlib.get_all_records(cursor)
      
      # check snapshot
      self.check_snapshot(expect_result, act_result)
      group_obj = self.db.get_replica_group_by_name(self.group_name)
      dataDB = group_obj.get_master().connect()

      self.create_statis_info(dataDB)
      self.checkDatabaseSnap(dataDB)
      self.db.reset_snapshot({ "Type": "database" })
      self.isDatabaseSnapClean(dataDB)
      
      #close dataDB
      dataDB.disconnect()
		
   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)

   def check_snapshot(self, expect_result, act_result):
      is_has_totaldataread = False
      is_has_totalindexread = False
      for x in act_result:
         if expect_result[0] in x:
            is_has_totaldataread = True
         if expect_result[1] in x:
            is_has_totalindexread = True
         self.assertTrue(is_has_totaldataread)
         self.assertTrue(is_has_totalindexread)
         
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
      
   def isDatabaseSnapClean(self, dataDB):
      # 6: database's snapshot
      cur = dataDB.get_snapshot(6)
      rec = cur.next()
      totalRead = rec["TotalRead"]
      cur.close()
      self.assertEqual(totalRead, 0)
      
   def checkDatabaseSnap(self, dataDB):
      # 6: database's snapshot
      cur = dataDB.get_snapshot(6)
      rec = cur.next()
      totalRead = rec["TotalRead"]
      cur.close()
      self.assertNotEqual(totalRead, 0)
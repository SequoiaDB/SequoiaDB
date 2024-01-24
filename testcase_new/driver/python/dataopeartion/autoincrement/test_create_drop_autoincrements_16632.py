# @decription create and drop autoincrements
# @testlink   seqDB-16632
# @interface  create_autoincrement	( self, options )  drop_autoincrement( self, names )
# @author     yinzhen 2018-12-12

from lib import testlib
from lib import sdbconfig
from dataopeartion.autoincrement.commlib import *
from pysequoiadb.error import (SDBBaseError, SDBEndOfCursor)


class TestCreateDropAutoIncrements16632(testlib.SdbTestBase):
   def setUp(self):
      #skip standlone mode
      if testlib.is_standalone():
         self.skipTest("skip! This testcase do not support standlone")
      
      # create cs cl   
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)      
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)

   def test_create_drop_autoincrements_16632(self):
      #create autoincrement with no paramter or None
      try:
         self.cl.create_autoincrement({})
      except SDBBaseError as e:
         self.assertEqual(-6, e.code)
         
      try:
         self.cl.create_autoincrement(None)
      except SDBBaseError as e:
         self.assertEqual(-6, e.code)
      
      #create autoincrement with invalid paramter
      try:
         self.cl.create_autoincrement({"FirstField":"a", "Increment":"10"})
      except SDBBaseError as e:
         self.assertEqual(-6, e.code)
      
      #create autoincrement with effective paramter
      options = {"Increment":1, "StartValue":1, "MinValue":1, "MaxValue":9223372036854775807, "CacheSize":1000, "AcquireSize":1000, "Cycled":False}
      self.cl.create_autoincrement([{"Field":"a"},{"Field":"b"},{"Field":"c"}])
      self.cl.create_autoincrement(({"Field":"d"},{"Field":"e"},{"Field":"f"}))
      self.assertTrue(check_autoincrement(self.db, self.cs_name + "." + self.cl_name, "a", options))
      self.assertTrue(check_autoincrement(self.db, self.cs_name + "." + self.cl_name, "b", options))
      self.assertTrue(check_autoincrement(self.db, self.cs_name + "." + self.cl_name, "c", options))
      self.assertTrue(check_autoincrement(self.db, self.cs_name + "." + self.cl_name, "d", options))
      self.assertTrue(check_autoincrement(self.db, self.cs_name + "." + self.cl_name, "e", options))
      self.assertTrue(check_autoincrement(self.db, self.cs_name + "." + self.cl_name, "f", options))
      
      #insert
      self.insert_records() 
      except_records = self.get_expect_records()
      cursor = self.cl.query()
      actual_records = testlib.get_all_records_noid(cursor)
      msg = str(except_records) + "expect is not equal to actResult" + str(actual_records)
      self.assertListEqual(except_records, actual_records, msg)
      
      #drop_autoincrement
      try:
         self.cl.drop_autoincrement("test")
      except SDBBaseError as e:
         self.assertEqual(-333, e.code)
         
      try:
         self.cl.drop_autoincrement("")
      except SDBBaseError as e:
         self.assertEqual(-6, e.code)
         
      try:
         self.cl.drop_autoincrement(None)
      except SDBBaseError as e:
         self.assertEqual(-6, e.code)
         
      #check sequence exists
      self.assertTrue(check_autoincrement_exist(self.db, self.cs_name + "." + self.cl_name, "a"))
      self.assertTrue(check_autoincrement_exist(self.db, self.cs_name + "." + self.cl_name, "b"))
      self.cl.drop_autoincrement(["a", "b"])
      self.assertFalse(check_autoincrement_exist(self.db, self.cs_name + "." + self.cl_name, "a"))
      self.assertFalse(check_autoincrement_exist(self.db, self.cs_name + "." + self.cl_name, "b"))
      
      self.assertTrue(check_autoincrement_exist(self.db, self.cs_name + "." + self.cl_name, "c"))
      self.assertTrue(check_autoincrement_exist(self.db, self.cs_name + "." + self.cl_name, "d"))
      self.cl.drop_autoincrement(("c", "d"))
      self.assertFalse(check_autoincrement_exist(self.db, self.cs_name + "." + self.cl_name, "c"))
      self.assertFalse(check_autoincrement_exist(self.db, self.cs_name + "." + self.cl_name, "d"))
      
   def tearDown(self):
      self.db.drop_collection_space(self.cs_name)   
      self.db.disconnect()
      
   def insert_records(self):
      doc = []
      for i in range(0, 10):
         doc.append({"name":"a"+str(i)})
      try:
         flags = 0
         self.cl.bulk_insert(flags, doc)
      except SDBBaseError as e:
         self.fail('insert fail: ' + str(e))

   def get_expect_records(self):
      doc = []
      for i in range(0, 10):
         doc.append({"name":"a"+str(i), "a":i+1, "b":i+1, "c":i+1, "d":i+1, "e":i+1, "f":i+1})
      return doc
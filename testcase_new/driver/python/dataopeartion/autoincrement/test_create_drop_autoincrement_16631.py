# @decription create and drop autoincrement
# @testlink   seqDB-16631
# @interface  create_autoincrement	( self, options )  drop_autoincrement( self, names )
# @author     yinzhen 2018-12-07

from lib import testlib
from lib import sdbconfig
from dataopeartion.autoincrement.commlib import *
from pysequoiadb.error import (SDBBaseError, SDBEndOfCursor)


class TestCreateDropAutoIncrement16631(testlib.SdbTestBase):
   def setUp(self):
      #skip standlone mode
      if testlib.is_standalone():
         self.skipTest("skip! This testcase do not support standlone")
   
      # create cs cl   
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)      
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)

   def test_create_drop_autoincrement_16631(self):
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
         self.cl.create_autoincrement({"FirstField":"age", "Increment":"10"})
      except SDBBaseError as e:
         self.assertEqual(-6, e.code)
      
      #create autoincrement with effective paramter
      self.cl.create_autoincrement({"Field":"age", "Increment":2, "StartValue":12, "MinValue":10, "MaxValue":500, "CacheSize":100, "AcquireSize":50, "Cycled":True, "Generated":"strict"})
      options = {"Increment":2, "StartValue":12, "MinValue":10, "MaxValue":500, "CacheSize":100, "AcquireSize":50, "Cycled":True}
      self.assertTrue(check_autoincrement(self.db, self.cs_name + "." + self.cl_name, "age", options))
      self.cl.create_autoincrement({"Field":"number"})
      options = {"Increment":1, "StartValue":1, "MinValue":1, "MaxValue":9223372036854775807, "CacheSize":1000, "AcquireSize":1000, "Cycled":False}
      self.assertTrue(check_autoincrement(self.db, self.cs_name + "." + self.cl_name, "number", options))
      
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
         
      #check sequence exist
      self.assertTrue(check_autoincrement_exist(self.db, self.cs_name + "." + self.cl_name, "age"))
      self.cl.drop_autoincrement("age")
      self.assertFalse(check_autoincrement_exist(self.db, self.cs_name + "." + self.cl_name, "age"))
      
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
         doc.append({"name":"a"+str(i), "age":12+2*i, "number":i+1})
      return doc 
# @decription create collection with autoincrement then alter autoincrement
# @testlink   seqDB-16655
# @interface  create_collection ( self, cl_name, options = None ),alter ( self, options ),set_attributes ( self, options )
# @author     yinzhen 2018-12-12

from lib import testlib
from lib import sdbconfig
from dataopeartion.autoincrement.commlib import *
from pysequoiadb.error import (SDBBaseError, SDBEndOfCursor)


class TestAlterAutoIncrement16655(testlib.SdbTestBase):
   def setUp(self):
      #skip standlone mode
      if testlib.is_standalone():
         self.skipTest("skip! This testcase do not support standlone")
   
      # create cs cl   
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)      
      self.cs = self.db.create_collection_space(self.cs_name)

   def test_sequence_snapshot_list_16655(self):
      #create autoincrement
      self.cl = self.cs.create_collection(self.cl_name, {"AutoIncrement":{"Field":"test16655"}})

      #check snapshot
      self.assertTrue(check_autoincrement_exist(self.db, self.cs_name + "." + self.cl_name, "test16655"))
      options = {"Increment":1, "StartValue":1, "MinValue":1, "MaxValue":9223372036854775807, "CacheSize":1000, "AcquireSize":1000, "Cycled":False}
      self.assertTrue(check_autoincrement(self.db, self.cs_name + "." + self.cl_name, "test16655", options))
      
      #alter autoincrement
      self.cl.alter({"AutoIncrement":{"Field":"test16655", "Increment":2, "StartValue":12, "MinValue":10, "MaxValue":500, "CacheSize":100, "AcquireSize":50}})
      options = {"Increment":2, "StartValue":12, "MinValue":10, "MaxValue":500, "CacheSize":100, "AcquireSize":50, "Cycled":False}
      self.assertTrue(check_autoincrement(self.db, self.cs_name + "." + self.cl_name, "test16655", options))
      
      #set_attributes autoincrement
      self.cl.set_attributes({"AutoIncrement":{"Field":"test16655", "Increment":3, "StartValue":100, "MinValue":1, "MaxValue":50000, "CacheSize":1000, "AcquireSize":800}})
      options = {"Increment":3, "StartValue":100, "MinValue":1, "MaxValue":50000, "CacheSize":1000, "AcquireSize":800, "Cycled":False}
      self.assertTrue(check_autoincrement(self.db, self.cs_name + "." + self.cl_name, "test16655", options))
      
   def tearDown(self):
      self.db.drop_collection_space(self.cs_name)   
      self.db.disconnect()
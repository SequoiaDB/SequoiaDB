# @decription test sequence get_snapshot and get_list interface
# @testlink   seqDB-16633
# @interface  get_snapshot ( self, snap_type, kwargs ) get_list ( self, list_type, kwargs )
# @author     yinzhen 2018-12-12

from lib import testlib
from lib import sdbconfig
from dataopeartion.autoincrement.commlib import *
from pysequoiadb.error import (SDBBaseError, SDBEndOfCursor)


class TestSequenceSnapshotList16633(testlib.SdbTestBase):
   def setUp(self):
      #skip standlone mode
      if testlib.is_standalone():
         self.skipTest("skip! This testcase do not support standlone")
   
      # create cs cl   
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)      
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)

   def test_sequence_snapshot_list_16633(self):
      #create autoincrement
      self.cl.create_autoincrement({"Field":"test16633", "Increment":2, "StartValue":12, "MinValue":10, "MaxValue":500, "CacheSize":100, "AcquireSize":50, "Cycled" : True, "Generated":"strict"})
         
      #check snapshot
      self.assertTrue(check_autoincrement_exist(self.db, self.cs_name + "." + self.cl_name, "test16633"))
      options = {"Increment":2, "StartValue":12, "MinValue":10, "MaxValue":500, "CacheSize":100, "AcquireSize":50, "Cycled" : True}
      self.assertTrue(check_autoincrement(self.db, self.cs_name + "." + self.cl_name, "test16633", options))
      
      #check list
      cursor = self.db.get_list(15)
      list_sequence = testlib.get_all_records(cursor)
      sequence_name = get_sequence_name(self.db, self.cs_name + "." + self.cl_name, "test16633")
      self.assertTrue(self.check_sequence_name_in_list(list_sequence, sequence_name), str(list_sequence) + " sequence_name : " + sequence_name )
            
      
   def tearDown(self):
      self.db.drop_collection_space(self.cs_name)   
      self.db.disconnect()
      
   def check_sequence_name_in_list(self, list_sequence, sequence_name):
      for item in list_sequence:
         if (item["Name"] != sequence_name):
            continue
         else:
            return True
      return False
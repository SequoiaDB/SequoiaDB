# @decription: create exist coord/catalog/data group, get/drop non exist data group
# @testlink:   seqDB-13687
# @author:     liuxiaoxuan 2017-12-07

from lib import testlib
from lib import sdbconfig
from bson.py3compat import (long_type)
from pysequoiadb.error import (SDBBaseError, SDBError)

class groupException13687(testlib.SdbTestBase):
   def setUp(self):
      self.group_name = "group13687"
      if testlib.is_standalone():
         self.skipTest('skip standalone') 

   def test_group_13687(self):
      # check create exist coord
      self.check_error_create_coord()
      
      # check create exist catalog group
      cata_master_node = self.db.get_cata_replica_group().get_master()
      host_name = cata_master_node.get_hostname()
      svc_name = cata_master_node.get_servicename()
      path = sdbconfig.sdb_config.rsrv_node_dir + svc_name
      self.check_error_create_cata(host_name, svc_name, path)
      
      # create non exist data group
      self.db.create_replica_group(self.group_name)
      
      # check data group created success
      self.check_create_group_success(self.group_name)
      
      # check create exist data group
      self.check_error_create_data_group(self.group_name)
      
      # drop exist group
      self.db.remove_replica_group(self.group_name)
      
      # check group removed success
      self.check_remove_group_success(self.group_name)
      
      # check drop non exist data group
      self.check_error_remove_data_group(self.group_name)
      
   def check_error_create_coord(self):
      try:
         self.db.create_coord_replica_group()     
         self.fail("NEED SDB ERROR")         
      except SDBError as e:
         self.assertEqual(e.code, -153)
         self.assertEqual(e.detail, "Failed to create replica group: SYSCoord") 
         
   def check_error_create_cata(self, host_name, svc_name, path):
      try:
         self.db.create_cata_replica_group(host_name, svc_name, path)     
         self.fail("NEED SDB ERROR")         
      except SDBError as e:
         self.assertEqual(e.code, -200)
         self.assertEqual(e.detail, "Failed to create catalog group")     

   def check_error_create_data_group(self, group_name):
      try:
         self.db.create_replica_group(group_name)     
         self.fail("NEED SDB ERROR")         
      except SDBError as e:
         self.assertEqual(e.code, -153)
         self.assertEqual(e.detail, "Failed to create replica group: " + group_name) 

   def check_error_remove_data_group(self, group_name):
      try:
         self.db.remove_replica_group(group_name)     
         self.fail("NEED SDB ERROR")         
      except SDBError as e:
         self.assertEqual(e.code, -154)
         self.assertEqual(e.detail, "Failed to remove replica group: " + group_name)         

   def check_create_group_success(self, group_name):
      try:
         self.db.get_replica_group_by_name(group_name)     
      except SDBBaseError as e:
         self.fail("check create group fail: " + str(e))
         
   def check_remove_group_success(self, group_name):
      try:
         self.db.get_replica_group_by_name(group_name)   
         self.fail("CHECK REMOVE RG FAIL")         
      except SDBBaseError as e:
         self.assertEqual(e.code, -154)
         
   def tearDown(self):
      try:
         self.db.remove_replica_group(self.group_name)       
      except SDBBaseError as e:
         if -154 != e.code:
            self.fail("tear down fail: " + str(e))
         
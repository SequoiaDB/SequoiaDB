# @decription: create exist node, get/connect/remove non exist node
# @testlink:   seqDB-13689/seqDB-13692
# @author:     liuxiaoxuan 2017-12-07

from lib import testlib
from lib import sdbconfig
from pysequoiadb.error import (SDBBaseError, SDBError, SDBNetworkError)

class nodeException13689(testlib.SdbTestBase):
   def setUp(self):
      self.group_name = "group13689"
      if testlib.is_standalone():
         self.skipTest('skip standalone') 

   def test_node_13689(self):
      # create non exist data group
      self.db.create_replica_group(self.group_name)
      
      # get data group
      data_rg = self.db.get_replica_group_by_name(self.group_name)
      
      # create node 1
      data_hostname = self.db.get_replica_group_by_name("SYSCatalogGroup").get_master().get_hostname()
      service_name1 = str(sdbconfig.sdb_config.rsrv_port_begin)
      data_dbpath1 = sdbconfig.sdb_config.rsrv_node_dir + service_name1
      data_rg.create_node(data_hostname, service_name1, data_dbpath1)
      
      # create node 2
      service_name2 = str(sdbconfig.sdb_config.rsrv_port_end)
      data_dbpath2 = sdbconfig.sdb_config.rsrv_node_dir + service_name2
      data_rg.create_node(data_hostname, service_name2, data_dbpath2)
      
      # check create node success
      self.check_create_node_success(data_rg, data_hostname, service_name2)
      
      # check create exist node 
      self.check_error_create_node(data_rg, data_hostname, service_name1, data_dbpath1)
      self.check_error_create_node(data_rg, data_hostname, service_name2, data_dbpath2)
      
      # remove exist node
      data_rg.remove_node(data_hostname, service_name2)
      
      # check node removed success
      self.check_remove_node_success(data_rg, data_hostname, service_name2)
      
      # check remove non exist node
      self.check_error_remove_node(data_rg, data_hostname, service_name2)
      
      # connect to removed node
      self.check_error_connect_node(data_hostname, service_name2)
      
       # remove data group
      self.db.remove_replica_group(self.group_name) 
  
   def check_error_create_node(self, data_rg, host_name, svc_name, dbpath):
      try:
         data_rg.create_node(host_name, svc_name, dbpath)     
         self.fail("NEED SDB ERROR")         
      except SDBError as e:
         self.assertEqual(e.code, -145)
         self.assertEqual(e.detail, "Failed to create node")    

   def check_error_connect_node(self, host_name, svc_name):
      try:
         config = sdbconfig.SdbConfig()
         new_db = testlib.client(config.host_name, config.service)
         new_db.connect(host_name, svc_name)     
         self.fail("NEED NETWORK ERROR")         
      except SDBNetworkError as e:
         self.assertEqual(e.code, -79)
         self.assertEqual(e.detail, "Failed to connect to " + host_name + ":" + svc_name)

   def check_error_remove_node(self, data_rg, host_name, svc_name):
      try:
         data_rg.remove_node(host_name, svc_name)     
         self.fail("NEED SDB ERROR")         
      except SDBError as e:
         self.assertEqual(e.code, -155)
         self.assertEqual(e.detail, "Failed to remove node")          
   
   def check_create_node_success(self, data_rg, host_name, svc_name):
      try:
         data_rg.get_nodebyendpoint(host_name, svc_name)        
      except SDBBaseError as e:
         self.fail("check create node fail: " + str(e))
         
   def check_remove_node_success(self, data_rg, host_name, svc_name):
      try:
         data_rg.get_nodebyendpoint(host_name, svc_name)
         self.fail("CHECK REMOVE NODE FAIL")         
      except SDBBaseError as e:
         self.assertEqual(e.code, -155)
         
   def tearDown(self):
      try:
         self.db.remove_replica_group(self.group_name)       
      except SDBBaseError as e:
         if -154 != e.code:
            self.fail("tear down fail: " + str(e))
         
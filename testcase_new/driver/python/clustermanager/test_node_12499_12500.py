# @decription: attach/detach data node
# @testlink:   seqDB-12499/seqDB-12500
# @interface:  attach_node
#              detach_node
# @author:     zhaoyu 2017-9-9

import unittest
import time
from lib import testlib
from lib import sdbconfig
from clustermanager import commlib
from pysequoiadb import client
from pysequoiadb.error import SDBBaseError

class TestDataNode12499(testlib.SdbTestBase):
   def setUp(self):
      # check standalone
      if testlib.is_standalone():
         self.skipTest('run mode is standalone')
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.data_rg_name = "data12499"

   def test_data_node_12499(self): 
      # create data rg
      data_rg = self.db.create_replica_group(self.data_rg_name)
      
      # create SYSSpare
      spare_rg = self.db.create_replica_group("SYSSpare")
      
      # create node 1
      data_hostname = self.db.get_replica_group_by_name("SYSCatalogGroup").get_master().get_hostname()
      service_name1 = str(sdbconfig.sdb_config.rsrv_port_begin)
      data_dbpath1 = sdbconfig.sdb_config.rsrv_node_dir + service_name1
      data_rg.create_node(data_hostname, service_name1, data_dbpath1)

      # create node 2
      service_name2 = str(sdbconfig.sdb_config.rsrv_port_begin + 10)
      data_dbpath2 = sdbconfig.sdb_config.rsrv_node_dir + service_name2
      config = {"logfilesz": 128}
      data_rg.create_node(data_hostname, service_name2, data_dbpath2, config)
      
      # rg start
      data_rg.start()

      commlib.check_rg_master( data_rg )
      
      rg_master = data_rg.get_master()
      rg_slave = data_rg.get_slave()
      master_data_connect_status = commlib.check_data_start_status(rg_master)
      slave_data_connect_status = commlib.check_data_start_status(rg_slave)
      self.assertTrue(master_data_connect_status)
      self.assertTrue(slave_data_connect_status)
      
      #create cs cl
      cs = self.db.create_collection_space(self.cs_name)
      cl = cs.create_collection(self.cl_name, {"Group":self.data_rg_name})
      
      #get cl from slave node
      slave_node = rg_slave.connect()
      cl_full_name = self.cl_name_qualified
      self.assertTrue(self.get_cl_from_slave_node(slave_node, cl_full_name))
      
      # detach node
      data_rg_slave_service = data_rg.get_slave().get_servicename()
      data_rg.detach_node(data_hostname, data_rg_slave_service, {"KeepData": True})
      
      # attach node with config
      spare_rg.attach_node(data_hostname, data_rg_slave_service, {"KeepData": True})
      spare_rg.start()
            
      # check data
      spare_data = client(data_hostname, data_rg_slave_service)
      get_full_name = spare_data.get_collection(cl_full_name).get_full_name()
      self.assertEqual(get_full_name, cl_full_name)
            
      #dropcs from catalog
      self.db.drop_collection_space(self.cs_name)
      
      # remove node
      self.db.remove_replica_group(self.data_rg_name)
      self.db.remove_replica_group("SYSSpare")
   
   def tearDown(self):
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.remove_rg(self.data_rg_name)
      self.remove_rg('SYSSpare')
      self.db.disconnect()
      
   def remove_rg(self, rg):
      try:
         self.db.remove_replica_group(rg)
      except SDBBaseError as e:
         if -154 != e.code:
            self.fail(e.code)
            
   def get_cl_from_slave_node(self, data_node, cl_full_name):
      get_cl_flag = False
      for i in range(10):
         try:
            cl = data_node.get_collection(cl_full_name)
            get_cl_flag = True
            break
         except SDBBaseError as e:
            if -23 != e.code and -34 != e.code :
               self.fail(e.code)
            else:
               print("get cl from slave %d times"%i)
               time.sleep(2)
               continue
      return get_cl_flag

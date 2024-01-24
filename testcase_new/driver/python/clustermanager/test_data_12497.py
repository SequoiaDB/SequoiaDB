# @decription: data rg
# @testlink:   seqDB-12497
# @interface:  create_replica_group
#              get_replica_group_by_id
#              get_replica_group_by_name
#              list_replica_groups
#              remove_replica_group
#              get_detail
#              rg.start
#              rg.stop
# @author:     zhaoyu 2017-9-8

import unittest
from lib import testlib
from lib import sdbconfig
from clustermanager import commlib
from pysequoiadb.error import SDBBaseError

class TestDataRg12497(testlib.SdbTestBase):
   def setUp(self):
      # check standalone
      if testlib.is_standalone():
         self.skipTest('run mode is standalone')
      self.data_rg_name = "data12497"
      
   def test_data_12497(self):
      # create data rg
      data_rg = self.db.create_replica_group(self.data_rg_name)
      
      #check list_replica_groups
      data_rgs = commlib.get_data_groups(self.db)
      if not self.data_rg_name in data_rgs:
         self.fail("create data group fail: " + str(data_rgs))
      
      #create node 1
      data_hostname = self.db.get_replica_group_by_name("SYSCatalogGroup").get_master().get_hostname()
      service_name1 = str(sdbconfig.sdb_config.rsrv_port_begin)
      data_dbpath1 = sdbconfig.sdb_config.rsrv_node_dir + service_name1
      data_rg.create_node(data_hostname, service_name1, data_dbpath1)

      #create node 2
      service_name2 = str(sdbconfig.sdb_config.rsrv_port_begin + 10)
      data_dbpath2 = sdbconfig.sdb_config.rsrv_node_dir + service_name2
      config = { "logfilesz": 128}
      data_rg.create_node(data_hostname, service_name2, data_dbpath2, config)
      
      #start rg and check
      data_rg= self.db.get_replica_group_by_name(self.data_rg_name)
      data_rg.start()
     
      # check master node 
      commlib.check_rg_master(data_rg)
      
      rg_master = data_rg.get_master()
      rg_slave = data_rg.get_slave()
      master_data_connect_status = commlib.check_data_start_status(rg_master)
      slave_data_connect_status = commlib.check_data_start_status(rg_slave)
      self.assertTrue(master_data_connect_status)
      self.assertTrue(slave_data_connect_status)
      
      #stop rg and check
      group_detail = data_rg.get_detail()
      data_id = group_detail["GroupID"]
      data_rg = self.db.get_replica_group_by_id(data_id)
      data_rg.stop()
      
      master_data_connect_status = commlib.check_data_stop_status(rg_master)
      slave_data_connect_status = commlib.check_data_stop_status(rg_slave)
      self.assertTrue(master_data_connect_status)
      self.assertTrue(slave_data_connect_status)
      
      #remove rg
      self.db.remove_replica_group(self.data_rg_name)
   
   def tearDown(self):
      try:
         self.db.remove_replica_group(self.data_rg_name)
         self.db.disconnect()
      except SDBBaseError as e:
         if -154 != e.code:
            self.fail("tear_down_fail:" + str(e))
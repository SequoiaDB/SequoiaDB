# @decription: test get_slave optimization, specify position
# @testlink:   seqDB-13794/13795/13796
# @interface:  get_slave(self, positions)
# @author:     liuxiaoxuan 2017-12-15

import unittest
import random
from lib import testlib
from lib import sdbconfig
from clustermanager import commlib
from pysequoiadb.error import SDBBaseError

class TestGetSlave13794(testlib.SdbTestBase):
   def setUp(self):
      if testlib.is_standalone():
         self.skipTest('skip standalone')
      self.one_node_group_name = "data_onenode_13793"
      self.more_node_group_name = "data_morenode_13793"

   def test_get_slave_13794(self):
      # create data rg
      one_node_data_rg = self.db.create_replica_group(self.one_node_group_name)
      more_node_data_rg = self.db.create_replica_group(self.more_node_group_name)

      # create node 
      data_hostname = self.db.get_replica_group_by_name("SYSCatalogGroup").get_master().get_hostname() 
      service_name = str(sdbconfig.sdb_config.rsrv_port_begin)
      data_dbpath = sdbconfig.sdb_config.rsrv_node_dir + service_name
      one_node_data_rg.create_node(data_hostname, service_name, data_dbpath)
      
      service_name1 = str(sdbconfig.sdb_config.rsrv_port_begin + 10)
      data_dbpath1 = sdbconfig.sdb_config.rsrv_node_dir + service_name1
      more_node_data_rg.create_node(data_hostname, service_name1, data_dbpath1)
      
      service_name2 = str(sdbconfig.sdb_config.rsrv_port_begin + 20)
      data_dbpath2 = sdbconfig.sdb_config.rsrv_node_dir + service_name2
      more_node_data_rg.create_node(data_hostname, service_name2, data_dbpath2)
      
      service_name3 = str(sdbconfig.sdb_config.rsrv_port_begin + 30)
      data_dbpath3 = sdbconfig.sdb_config.rsrv_node_dir + service_name3
      more_node_data_rg.create_node(data_hostname, service_name3, data_dbpath3)

      # start group
      one_node_data_rg.start()
      more_node_data_rg.start()
      
      # check master node 
      commlib.check_rg_master(one_node_data_rg)
      commlib.check_rg_master(more_node_data_rg)
         
      # get slave with one position
      one_position = random.randint(1,7)
      slave_one_node = one_node_data_rg.get_slave(one_position)
      self.assertTrue(self.is_master(one_node_data_rg, slave_one_node))
         
      slave_more_node = more_node_data_rg.get_slave(one_position)
      self.assertIsNotNone(slave_more_node)
      
      # get slave with more positions
      slave_one_node = one_node_data_rg.get_slave(1,2,3)
      self.assertTrue(self.is_master(one_node_data_rg, slave_one_node))
         
      slave_more_node = more_node_data_rg.get_slave(1,2,3)
      self.assertFalse(self.is_master(more_node_data_rg, slave_more_node))
      
      slave_more_node = more_node_data_rg.get_slave(1,4,6)
      self.assertFalse(self.is_master(more_node_data_rg, slave_more_node))
      
      # check invalid position
      positions = [0, 8, 1.1, '1', None]
      for pos in positions:
         print("position: " + str(pos))
         self.check_invalid_position(one_node_data_rg, pos)
         
      # remove data rg
      self.db.remove_replica_group(self.one_node_group_name)
      self.db.remove_replica_group(self.more_node_group_name)
   
   def tearDown(self):
      msg = "tear down fail: "
      self.remove_data_rg(self.one_node_group_name, msg)
      self.remove_data_rg(self.more_node_group_name, msg)
      self.db.disconnect()  
  
   def remove_data_rg(self, group_name, msg):
      try:
         self.db.remove_replica_group(group_name)
      except SDBBaseError as e:
         if -154 != e.code:
            self.fail(msg + str(e))
          
   def is_master(self, data_rg, node):
      is_master = False
      master_node = data_rg.get_master()
      if master_node.get_hostname() == node.get_hostname() \
            and master_node.get_servicename() == node.get_servicename():
         is_master = True           
      return is_master 

   def check_invalid_position(self, data_rg, position):
      try:
         data_rg.get_slave(position)
      except SDBBaseError as e:
         self.assertEqual(-6, e.code)
# @decription: data node
# @testlink:   seqDB-12498
# @interface:  create_node
#              remove_node
#              get_master
#              get_slave
#              get_nodebyendpoint
#              get_nodebyname
#              get_nodenum
#              connect
#              get_hostname
#              get_nodename
#              get_servicename
#              get_status
#              start
#              stop
# @author:     zhaoyu 2017-9-8

import unittest
from lib import testlib
from lib import sdbconfig
from clustermanager import commlib
from pysequoiadb.error import SDBBaseError

class TestDataNode12498(testlib.SdbTestBase):
   def setUp(self):
      # check standalone
      if testlib.is_standalone():
         self.skipTest('run mode is standalone')
      self.data_rg_name = "data12498"

   def test_data_node_12498(self):
      # create data rg
      data_rg = self.db.create_replica_group(self.data_rg_name)

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

      # start node
      node1 = data_rg.get_nodebyendpoint(data_hostname, service_name1)
      #node1 = data_rg.get_nodebyname(data_hostname + ":" + service_name1)
      node2 = data_rg.get_nodebyname(data_hostname + ":" + service_name2)
      node1.start()
      node2.start()

      commlib.check_rg_master(data_rg)
      
      rg_master = data_rg.get_master()
      rg_slave = data_rg.get_slave()
      
      master_data_connect_status = commlib.check_data_start_status(rg_master)
      slave_data_connect_status = commlib.check_data_start_status(rg_slave)
      self.assertTrue(master_data_connect_status)
      self.assertTrue(slave_data_connect_status)
      
      #stop node
      node1.stop()
      node2.stop()
           
      master_data_connect_status = commlib.check_data_stop_status(rg_master)
      slave_data_connect_status = commlib.check_data_stop_status(rg_slave)
      self.assertTrue(master_data_connect_status)
      self.assertTrue(slave_data_connect_status)
      
      #get node num and check
      node_num = data_rg.get_nodenum(1)
      self.assertEqual(node_num, 2)
      
      #get master and slave
      data_master_status = rg_master.get_status()
      self.assertEqual(data_master_status, 2)
      data_slave_status = rg_slave.get_status()
      self.assertEqual(data_slave_status, 2)
      
      #get node information and check
      node1_service_name = node1.get_servicename()
      self.assertEqual(node1_service_name, service_name1)
      node2_service_name = node2.get_servicename()
      self.assertEqual(node2_service_name, service_name2)
      
      node1_hostname = node1.get_hostname()
      self.assertEqual(node1_hostname, data_hostname)
      node2_hostname = node2.get_hostname()
      self.assertEqual(node2_hostname, data_hostname)
      
      node1_name =node1.get_nodename()
      self.assertEqual(node1_name, data_hostname + ":" + service_name1)
      node2_name = node2.get_nodename()
      self.assertEqual(node2_name, data_hostname + ":" + service_name2)
      
      #start node
      node1.start()
      node2.start()

      commlib.check_rg_master(data_rg)
      
      rg_master = data_rg.get_master()
      rg_slave = data_rg.get_slave()
      master_data_connect_status = commlib.check_data_start_status(rg_master)
      slave_data_connect_status = commlib.check_data_start_status(rg_slave)
      self.assertTrue(master_data_connect_status)
      self.assertTrue(slave_data_connect_status)
      
      # connect
      data1 = node1.connect()
      # check connect
      cs_name = "test_12498"
      try:
         data1.create_collection_space(cs_name)
         data1.drop_collection_space(cs_name)
      except SDBBaseError as e:
         if -33 != e.code and  -34 != e.code:
            self.fail("create and drop cs fail: " + str(e))
      
      # remove node
      host_name = rg_slave.get_hostname()
      svc_name = rg_slave.get_servicename()
      data_rg.remove_node(host_name, svc_name)
      # check node
      try:
         data_rg.get_nodebyendpoint(host_name,svc_name)
         self.fail("remove node fail")
      except SDBBaseError as e:
         if -155 != e.code:
            self.fail("remove node fail: " + str(e))
           
      # remove group
      self.db.remove_replica_group(self.data_rg_name)
      # check use list_replica_groups
      data_rgs = commlib.get_data_groups(self.db)
      if self.data_rg_name in data_rgs:
         self.fail("remove_rg_fail,data_rgs:" + str(data_rgs))
   
   def tearDown(self):
      try:
         self.db.remove_replica_group(self.data_rg_name)
         self.db.disconnect()
      except SDBBaseError as e:
         if -154 != e.code:
            self.fail("tear_down_fail:" + str(e))
          

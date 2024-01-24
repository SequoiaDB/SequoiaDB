# @decription: test update/delete config
# @testlink:   seqDB-15241
# @interface:  update_config(self, configs, options)
#              delete_config(self, configs, options)
# @author:     liuxiaoxuan 2018-8-10

import unittest
from lib import testlib
from lib import sdbconfig
from pysequoiadb.error import (SDBBaseError, SDBEndOfCursor)
SDB_SNAP_CONFIGS = 13
class TestClusterConfig15241(testlib.SdbTestBase):
   def setUp(self):
      # check standalone
      if testlib.is_standalone():
         self.skipTest('run mode is standalone')
      
   def test_cluster_config_15241(self):
      # get any of data rg
      groups = testlib.get_data_groups()
      data_group_name = groups[0]['GroupName']
      data_group = self.db.get_replica_group_by_name(data_group_name)  
		
		# get master node
      master = data_group.get_master()
      node_name = master.get_nodename()
      host_name = master.get_hostname()
      svc_name = master.get_servicename()

      # get default config before update
      node_snapshot = self.get_config_snapshot(condition = {"NodeName" : node_name})
      old_configs = {"weight" : node_snapshot[0]["weight"]}

      # update config
      new_configs = {"weight" : 99}
      options = {"HostName" : host_name, "svcname" : svc_name}
      self.db.update_config(new_configs, options)

      # check snaphsot after update config
      node_snapshot = self.get_config_snapshot(condition = {"NodeName" : node_name})
      act_result = {"weight" : node_snapshot[0]["weight"]}
      expect_result = {"weight" : 99}
      self.check_snapshot(expect_result, act_result)
		
      # delete config
      self.db.delete_config(new_configs, options)   

      # check snaphsot after delete config
      node_snapshot = self.get_config_snapshot(condition = {"NodeName" : node_name})
      act_result = {"weight" : node_snapshot[0]["weight"]}
      expect_result = old_configs
      self.check_snapshot(expect_result, act_result) 
		
   def tearDown(self):
      pass
		
   def get_config_snapshot(self, **kwargs):
      # get snapshot configs
      configs = list()	
      cursor = self.db.get_snapshot(SDB_SNAP_CONFIGS, **kwargs)
      while(True):
         try:
            configs.append(cursor.next())
         except SDBEndOfCursor:     		
            break
      return configs;
   	
   def check_snapshot(self, expect_result, act_result):
   	self.assertEqual(expect_result, act_result)
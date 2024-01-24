# @decription: data group
# @testlink:   seqDB-13486
# @interface:  start_replica_group(group_name)
#              stop_replica_group(group_name)
# @author:     liuxiaoxuan 2017-11-20

import unittest
from lib import testlib
from lib import sdbconfig
from pysequoiadb.error import SDBBaseError

class Testdata13486(testlib.SdbTestBase):
   def setUp(self): 
      # check standalone
      if testlib.is_standalone():
         self.skipTest('current environment is standalone')
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.group_name = "data13486"     
 
   def test_data_13486(self):
      # create data group  
      self.db.create_replica_group(self.group_name)
      
      # create nodes
      rg = self.db.get_replica_group_by_name(self.group_name)
      host_name = self.db.get_replica_group_by_name("SYSCatalogGroup").get_master().get_hostname()
      
      servic_name1 = str(sdbconfig.sdb_config.rsrv_port_begin)
      data_path1 = sdbconfig.sdb_config.rsrv_node_dir + servic_name1
      rg.create_node(host_name, servic_name1, data_path1)
      
      servic_name2 = str(sdbconfig.sdb_config.rsrv_port_end)
      data_path2 = sdbconfig.sdb_config.rsrv_node_dir + servic_name2
      rg.create_node(host_name, servic_name2, data_path2)
         
      # start group
      self.db.start_replica_group(self.group_name)
      
      # check group
      cs = self.db.create_collection_space(self.cs_name)
      cl = cs.create_collection(self.cl_name, {'Group' : self.group_name})
      
      # drop cs before group stop
      self.db.drop_collection_space(self.cs_name)
      
      # stop group
      self.db.stop_replica_group(self.group_name)
      
      # remove group
      self.db.remove_replica_group(self.group_name)
   
   def tearDown(self):
      try:
         self.db.remove_replica_group(self.group_name)
         self.db.disconnect()
      except SDBBaseError as e:
         if -154 != e.code:
            self.fail("remove rg fail when teardown: " + str(e))
      
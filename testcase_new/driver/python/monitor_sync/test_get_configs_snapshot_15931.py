# @decription appoint hint/num_to_skip/num_to_return to get SDB_SNAP_CONFIGS information
# @testlink   seqDB-15931
# @interface  get_snapshot	( self, snap_type, kwargs )		
# @author     yinzhen 2018-10-16

from lib import testlib
from lib import sdbconfig
from pysequoiadb.error import (SDBBaseError, SDBEndOfCursor)
import time

SDB_SNAP_COLLECTIONSPACES = 5
SDB_SNAP_CONFIGS = 13

class TestGetSnapshot15931(testlib.SdbTestBase):
   def setUp(self):
      pass
		 
   def test_get_snapshot_15931(self):
      data_rg_name = "data15931"
	
      # create data rg
      data_rg = self.db.create_replica_group(data_rg_name)
      
      # create and start node 
      data_hostname = self.db.get_replica_group_by_name("SYSCatalogGroup").get_master().get_hostname()
      service_name = str(sdbconfig.sdb_config.rsrv_port_begin)
      data_dbpath = sdbconfig.sdb_config.rsrv_node_dir + service_name
      data_rg.create_node(data_hostname, service_name, data_dbpath)
	  
      service_name = str(sdbconfig.sdb_config.rsrv_port_end)
      data_dbpath = sdbconfig.sdb_config.rsrv_node_dir + service_name
      data_rg.create_node(data_hostname, service_name, data_dbpath)
      data_rg.start()
     
      # check master node 
      check_rg_master(data_rg)

      # get_snapshot snap_type is SDB_SNAP_CONFIGS
      node = data_rg.get_master().connect()
      node.update_config(configs = {"optimeout" : 100000, "maxreplsync":0}, options = {"Global" : False})
      actResult = self.get_snapshot_result(node, SDB_SNAP_CONFIGS, condition = {"svcname":"27000"}, selector = {"NodeName":"", "svcname":"", "optimeout":"", "maxreplsync":""}, order_by = {"NodeName":-1}, hint = {"$Option":{"mode":"local", "expand":"false"}}, num_to_skip = 0, num_to_return = 1)
      
      # check result
      expResult = [{"NodeName":data_hostname + ":" + service_name, "svcname" : service_name, "optimeout" : 100000, "maxreplsync" : 0}] 
      msg = str(expResult) + "expect is not equal to actResult" + str(actResult)
      self.assertListEqual(expResult, actResult, msg)
      	  
      cs = node.create_collection_space(self.cs_name)
      cl = cs.create_collection("testcl15931", {'IsMainCL': True, 'ShardingKey': {'a':1}, 'ReplSize' : 0})
      	  
      # get_snapshot snap_type is SDB_SNAP_COLLECTIONSPACES
      actResult = self.get_snapshot_result(node, SDB_SNAP_COLLECTIONSPACES, condition = {"Name":self.cs_name}, selector = {"Name":"","Collection":{"$elemMatch":{"Name":self.cs_name + ".testcl15931"}}}, order_by = {"Name" : 1}, hint = {"$Option":{"mode":"local", "expand":"false"}}, num_to_skip =0, num_to_return =1)
      
      # check result
      expResult = [{"Name":self.cs_name, "Collection":[{'Name': 'type_cs.testcl15931', 'UniqueID': -1}]}]
      msg = str(expResult) + "expect is not equal to actResult" + str(actResult)
      self.assertListEqual(expResult, actResult, msg)
	  
      # drop data group      
      self.db.remove_replica_group(data_rg_name)
      self.db.disconnect()
	
   def tearDown(self):
      pass
	    
   def get_snapshot_result(self, node, snap_type, **kwargs):
      result = []
      cursor = node.get_snapshot(snap_type, **kwargs)
      while True:
         try:
            rc = cursor.next()
            result.append(rc) 
         except SDBEndOfCursor:
            break
      cursor.close()
      return result
	  
def check_rg_master( rg ):
   i = 0
   while True:
      try:
         print("get master times: " + str(i))
         time.sleep(3)
         rg.get_master()
         break
      except SDBBaseError as e:
         i = i + 1
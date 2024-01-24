# @decription check force session
# @testlink   seqDB-15698
# @interface  force_session(self,force_session, options=None)
# @author     liuxiaoxuan 2018-09-26

from lib import testlib
from lib import sdbconfig
from sdb_operate.commlib import *
from pysequoiadb.error import (SDBBaseError, SDBEndOfCursor)
import time

SDB_SNAP_SESSIONS=2
class TestForceSession15698(testlib.SdbTestBase):
   def setUp(self):
      # skip standalone
      if testlib.is_standalone():
         self.skipTest('run mode is standalone')
		     
   def test_force_session_15698(self):
      self.data_rg_name = "data15698"
	
      # create data rg
      data_rg = self.db.create_replica_group(self.data_rg_name)
      
      # create and start node 
      data_hostname = self.db.get_replica_group_by_name("SYSCatalogGroup").get_master().get_hostname()
      service_name = str(sdbconfig.sdb_config.rsrv_port_begin)
      data_dbpath = sdbconfig.sdb_config.rsrv_node_dir + service_name
      data_rg.create_node(data_hostname, service_name, data_dbpath)

      data_rg.start()
     
      # check master node 
      check_rg_master(data_rg)
      
		# get sessionid
      node_name = data_hostname + ":" + service_name
      session_ids = self.get_session_snapshot(condition = {"NodeName" : node_name})
      node = data_rg.get_master().connect()
      
     		
      # force session without options
      for session_id in session_ids:
         try:
            node.force_session(session_id)
            # check session has been killed, timeout 60s
            self.checkSession(session_id, condition = {"NodeName" : node_name})
            break
         except SDBBaseError as e:
            self.assertEquals(e.code, -63)
      
		# force session with options
      node = data_rg.get_master().connect()
      for session_id in session_ids:
         try:
            node.force_session(session_id, {"HostName" : data_hostname, "svcname" : service_name})
    	      # check session has been killed, timeout 60s
            self.checkSession(session_id, condition = {"NodeName" : node_name})
            break
         except SDBBaseError as e:
            if e.code != -63 and e.code != -62:
               self.assertFail(e.errcode)
	
   def tearDown(self):
      # drop data group
      self.db.remove_replica_group(self.data_rg_name)
      self.db.disconnect()
      
   def get_session_snapshot(self, **kwargs):
      session_ids = []
      cursor = self.db.get_snapshot(SDB_SNAP_SESSIONS, **kwargs)
      while True:
         try:
            rc = cursor.next()
            session_id = rc["SessionID"]
            session_ids.append(session_id) 
         except SDBEndOfCursor:
            break
      cursor.close()
      return session_ids
   
   def checkSession(self, session_id, **kwargs):
      i = 0
      while i < 60:
         session_ids = self.get_session_snapshot(**kwargs)
         if session_id in session_ids: 
            i = i + 1
            time.sleep(1)
         else:
            break
      self.assertNotIn(session_id, session_ids, "force current session failed")	
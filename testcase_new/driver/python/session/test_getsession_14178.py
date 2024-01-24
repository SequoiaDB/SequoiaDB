# @decription: get session attr
# @testlink:   seqDB-14178
# @interface:  set_session_attri(options)
#              get_session_attri()
# @author:     liuxiaoxuan 2018-01-25

import unittest
from lib import testlib
from lib import sdbconfig
from session.commlib import *
from pysequoiadb.error import SDBBaseError

class TestgetSession14178(testlib.SdbTestBase):
   def setUp(self):
      # check standalone
      if testlib.is_standalone():
         self.skipTest('run mode is standalone')
      self.data_rg_name = "session14178"
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist = True)

   def test_get_session_14178(self):
      # create data rg
      data_rg = self.db.create_replica_group(self.data_rg_name)

      # create node 1 with config {instanceid : 12}
      data_hostname = self.db.get_replica_group_by_name("SYSCatalogGroup").get_master().get_hostname()
      service_name1 = str(sdbconfig.sdb_config.rsrv_port_begin)
      data_dbpath1 = sdbconfig.sdb_config.rsrv_node_dir + service_name1
      config1 = {"instanceid ": 12}
      data_rg.create_node(data_hostname, service_name1, data_dbpath1, config1)

      # create node 2 with config {instanceid : 13}
      service_name2 = str(sdbconfig.sdb_config.rsrv_port_begin + 10)
      data_dbpath2 = sdbconfig.sdb_config.rsrv_node_dir + service_name2
      config2 = {"instanceid ": 13}
      data_rg.create_node(data_hostname, service_name2, data_dbpath2, config2)
      
      # create node 3 with config {instanceid : 14}
      service_name3 = str(sdbconfig.sdb_config.rsrv_port_begin + 20)
      data_dbpath3 = sdbconfig.sdb_config.rsrv_node_dir + service_name3
      config3 = {"instanceid ": 14}
      data_rg.create_node(data_hostname, service_name3, data_dbpath3, config3)

      # start group
      self.db.start_replica_group(self.data_rg_name)
      
      check_rg_master(data_rg)
      
      # create maincl and subcls in new group
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name , {'Group' : self.data_rg_name, 'ReplSize' : 0})
      
      # insert datas
      insert_nums = 10000
      self.insert_datas(insert_nums);
      
      # get default session attr
      act_session_attr = self.db.get_session_attri()
      expect_session_attr = {'PreferedInstance' : 'M', 'PreferedInstanceMode' : 'random', 'Timeout' : -1}
      self.check_session_attr(expect_session_attr, act_session_attr)
      
      # set session attr {"instanceid ": 12}
      opt = {'PreferedInstance' : 12};
      self.db.set_session_attri(options = opt)
      
      # check session attr
      act_session_attr = self.db.get_session_attri()
      expect_session_attr = {'PreferedInstance' : 12, 'PreferedInstanceMode' : 'random', 'Timeout' : -1}
      self.check_session_attr(expect_session_attr, act_session_attr)
      
      # set session attr {"instanceid ": 13}
      opt = {'PreferedInstance' : 13};
      self.db.set_session_attri(options = opt)
      
      # check session attr
      act_session_attr = self.db.get_session_attri()
      expect_session_attr = {'PreferedInstance' : 13, 'PreferedInstanceMode' : 'random', 'Timeout' : -1}
      self.check_session_attr(expect_session_attr, act_session_attr)
      
      # repeat get session attr
      for i in range(10):  
         self.check_session_attr(expect_session_attr, act_session_attr)
      
      # drop cs
      testlib.drop_cs(self.db, self.cs_name)
  
      # remove group
      self.db.remove_replica_group(self.data_rg_name)
   
   def tearDown(self):
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist = True)
      try:
         self.db.remove_replica_group(self.data_rg_name)
         self.db.disconnect()
      except SDBBaseError as e:
         if -154 != e.code:
            self.fail("tear_down_fail: " + str(e))
            
   def insert_datas( self , insert_nums):
      flag = 0
      doc = []
      for i in range(0, insert_nums):
         doc.append({"a": i})
      try:
         self.cl.bulk_insert(flag, doc)
      except SDBBaseError as e:   
         self.fail('insert fail: ' + str(e))
   
   def check_session_attr(self, expect_session_attr, act_session_attr):
      # compare act and expect results
      self.assertEqual(expect_session_attr['PreferedInstance'], act_session_attr['PreferedInstance'])
      self.assertEqual(expect_session_attr['PreferedInstanceMode'], act_session_attr['PreferedInstanceMode'])
      self.assertEqual(expect_session_attr['Timeout'], act_session_attr['Timeout']) 
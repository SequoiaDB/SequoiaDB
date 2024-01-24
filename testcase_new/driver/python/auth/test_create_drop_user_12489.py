# @decription: test create/remove user
# @testlink:   seqDB-12489
# @interface:  create_user(self,name,psw)
#              remove_user(self,name,psw)
# @author:     liuxiaoxuan 2017-9-08

import unittest
import datetime
from pysequoiadb.error import (SDBTypeError, SDBBaseError, SDBEndOfCursor, SDBError)
from pysequoiadb import client
from lib import sdbconfig
from lib import testlib
import random

username = "admin"
password = "admin"
insert_nums = 100
class TestCreateDropUsr12489(testlib.SdbTestBase):
   def setUp(self):
      if testlib.is_standalone():
         self.skipTest('current environment is standalone')   
		# get a local config	
      self.config = sdbconfig.SdbConfig()
		# get group
      dataGroups = testlib.get_data_groups()
      group = dataGroups[0]
      self.groupName = group["GroupName"]			

   def test_create_drop_user_12489(self):

      # create user
      self.check_create_user(username,password)
      # disconnect sdb
      self.db.disconnect()
      # reconnect sdb with user
      self.check_reconnect_db(username,password)
      # create cl
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      cl_option = {'ReplSize': 3 ,'Group': self.groupName}
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name,cl_option)
      # check insert
      self.insert_datas()
		# check connect catalog
      self.check_connect_catalog(username,password)
		# check connect node
      self.check_connect_node(username,password)
      # drop user
      self.db.remove_user(username, password)
      # check drop result
      self.check_drop_user(username,password)

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)	
         try:
            self.db.remove_user(username, password)
         except SDBBaseError as e:
            self.assertEqual(-300, e.code, "teardown fail,errmsg:" + str(e))
	
   # used to do connecting	
   def get_data_nodes(self):
      nodeAddrs = []
      try:   
         # get nodes
         rg = self.db.get_replica_group_by_name(self.groupName)
         rec = rg.get_detail()
         groups = rec['Group']

         for i in range(len(groups)):
            group = groups[i]
            host_name = group['HostName']
            service = group['Service']
            svc = service[0]
            svc_name = svc['Name']
            nodeAddrs.append({'host' : host_name , 'service': svc_name})
      except SDBBaseError as e:
         self.fail("get groupAdrr fail: " + str(e))
			
      return nodeAddrs		

   def insert_datas(self):
      flag = 0
      doc = []
      for i in range(0, insert_nums):
         doc.append({"a": "test" + str(i)})
      try:
         self.cl.bulk_insert(flag, doc)
      except SDBBaseError as e:
         self.fail('insert fail: ' + str(e))

   def check_create_user(self,username,password):
      try:
         self.db.create_user(username, password)
      except SDBBaseError as e:
         if(-295 != e.code):
            self.fail('create user fail: ' + str(e))

   def check_reconnect_db(self,username,password):
      try:
         self.db = client(self.config.host_name, self.config.service, username, password)
      except SDBBaseError as e:
         self.fail('reconnect with username fail: ' + str(e))
	
   # seqDB-12501, check connect(username,password)	
   def check_connect_catalog(self,username,password):
      new_db = testlib.client(self.config.host_name, self.config.service, username, password)
      try:
         cata_rg = new_db.get_replica_group_by_name('SYSCatalogGroup')
         cata_node = cata_rg.get_master().get_nodename()
         node_name = cata_node.split(":")

         hostname = node_name[0]
         svcname = node_name[1]
			
         new_db.connect(hostname,svcname,user = username,password = password)
      except SDBBaseError as e:
         self.fail("connect to catalog fail: " + str(e))
      finally:
         new_db.disconnect()	

   # seqDB-12501, check connect_to_hosts(username,password,kwargs)	
   def check_connect_node(self,username,password):
      new_db = testlib.client(self.config.host_name, self.config.service, username, password)
      try:
         repeatTime = 10
         hosts = self.get_data_nodes()

         for i in range(repeatTime):
            # choose a random policy option
            option = random.choice(['local_first','one_by_one','random'])
            # connect to a data node with option
            new_db.connect_to_hosts(hosts,user = username,password = password,policy = option)
				# check data result
            self.check_connect_result(new_db)
      except SDBBaseError as e:
         self.fail("connect to node fail: " + str(e))
      finally:
         new_db.disconnect()	
			
   def check_connect_result(self,node_db):
      try:
         # get new cl
         cl_full_name = self.cl_name_qualified
         new_cl = node_db.get_collection(cl_full_name)
         actCount = new_cl.get_count()
         self.assertEqual(insert_nums, actCount)
      except SDBBaseError as e:
         self.fail('check node fail: ' + str(e))
			
   def check_drop_user(self,username,password):
      try:
         self.db.remove_user(username, password)
         self.fail('NEED DROP USER FAIL')
      except SDBBaseError as e:
          self.assertEqual(-300, e.code, "error msg: " + str(e))

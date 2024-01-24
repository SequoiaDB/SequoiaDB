# @decription: test connect/disconnect node
# @testlink:   seqDB-12501
# @interface:  connect(self,host,service,kwargs )
#              connect_to_hosts(self,hostskwargs )
#              disconnect(self)
#              is_valid(self)
# @author:     liuxiaoxuan 2017-9-08

import random

from lib import testlib
from pysequoiadb.error import (SDBBaseError)

insert_nums = 100
class TestConnect12501(testlib.SdbTestBase):
   def setUp(self):
      if testlib.is_standalone():
         self.skipTest('current environment is standalone')

      dataGroups = testlib.get_data_groups()
      group = dataGroups[0]
      self.groupName = group["GroupName"]

      cl_option = {'ReplSize': 3, 'Group': self.groupName}
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist = True)
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name,options = cl_option)
      self.insert_datas()

   def test_connect_12501(self):
      # check catalog
      cl_full_name = self.cl_name_qualified
      condition = {'Name': cl_full_name}
      expectResult = {'Name': cl_full_name, 'ReplSize': 3, 'GroupName': self.groupName}
      self.get_catalog_info(condition, expectResult)

      # check connect data node
      self.check_connect_node()
      # check disconnect
      self.check_disconnect_node()

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)

   def insert_datas(self):
      flag = 0
      doc = []
      for i in range(0, insert_nums):
         doc.append({"a": "test" + str(i)})
      try:
         self.cl.bulk_insert(flag, doc)
      except SDBBaseError as e:
         self.fail('insert fail: ' + str(e))

   def get_catalog_info(self, cond, expectRec):
      new_db = testlib.default_db()
      try:
         # connect to catalog primary node
         cata_rg = new_db.get_replica_group_by_name('SYSCatalogGroup')
         cata_node = cata_rg.get_master().get_nodename()
         node_name = cata_node.split(":")

         hostname = node_name[0]
         svcname = node_name[1]
         new_db.connect(hostname, svcname)

         # check catalog info
         cl_full_name = self.cl_name_qualified
         cata_cl = new_db.get_collection('SYSCAT.SYSCOLLECTIONS')
         actRec = cata_cl.query(condition={'Name': cl_full_name}).next()

         self.check_catalog_result(expectRec, actRec)
      except SDBBaseError as e:
         self.fail('check catalog fail: ' + str(e))
      finally:
         new_db.disconnect()

   def check_catalog_result(self, expectRec, actRec):
      msg = str(expectRec) + " not equal " + str(actRec)
      catalog = actRec['CataInfo']
      info = catalog[0]
      self.assertEqual(expectRec['Name'], actRec['Name'], msg)
      self.assertEqual(expectRec['ReplSize'], actRec['ReplSize'], msg)
      self.assertEqual(expectRec['GroupName'], info['GroupName'], msg)

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
            nodeAddrs.append({'host': host_name, 'service': svc_name})

      except SDBBaseError as e:
         self.fail("get groupAdrr fail: " + str(e))

      return nodeAddrs

   def check_connect_node(self):
      new_db = testlib.default_db()
      try:
         repeatTime = 10
         hosts = self.get_data_nodes()

         for i in range(repeatTime):
            # choose a random policy option
            option = random.choice(['local_first', 'one_by_one', 'random'])
            # connect to a data node with option
            new_db.connect_to_hosts(hosts, policy=option)
            # check data result
            self.check_connect_result(new_db)
      except SDBBaseError as e:
         self.fail("connect to node fail: " + str(e))
      finally:
         new_db.disconnect()

   def check_connect_result(self, node_db):
      try:
         # get new cl
         cl_full_name = self.cl_name_qualified
         new_cl = node_db.get_collection(cl_full_name)
         actCount = new_cl.get_count()
         self.assertEqual(insert_nums, actCount)
      except SDBBaseError as e:
         self.fail('check node fail: ' + str(e))

   def check_disconnect_node(self):
      new_db = testlib.default_db()
      try:
         new_db.disconnect()
         # check disconnect
         expectRec = 0
         self.check_disconnect_result(new_db, expectRec)
      except SDBBaseError as e:
         self.fail('disconnect node fail: ' + str(e))

   def check_disconnect_result(self, new_db, expectRec):
      try:
         actRec = new_db.is_valid()
         self.assertEqual(expectRec, actRec, 'node still valid')
      except SDBBaseError as e:
         self.fail('check disconnect node fail: ' + str(e))

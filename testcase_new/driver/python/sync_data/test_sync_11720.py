# @decription: sync data
# @testlink:   seqDB-11720
# @interface:  sync(self,options = NONE)
# @author:     liuxiaoxuan 2020-03-05

from lib import testlib

class TestSyncData11720(testlib.SdbTestBase):
   def setUp(self):
      # skip standalone
      if testlib.is_standalone():
         self.skipTest('run mode is standalone')
         
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      # create cs
      self.cs = self.db.create_collection_space(self.cs_name)

   def test_sync_data_11720(self):
      # get group
      groups = testlib.get_data_groups()
      groupname = groups[0]['GroupName']
      # create cl
      self.cl = self.cs.create_collection(self.cl_name, {'Group': groupname, 'ReplSize': -1})

      data_rg= self.db.get_replica_group_by_name(groupname)
      group_detail = data_rg.get_detail()
      group_id = group_detail["GroupID"]   
      hostname = data_rg.get_master().get_hostname();
      svcname = data_rg.get_master().get_servicename();
      print("groupname:" + groupname + ",group_id:" + str(group_id) + ",hostname:" + hostname + ",svcname:" + str(svcname) + "csname: " + self.cs_name);      
     
      # sync
      option = { "Deep": 1, "Block": True, "CollectionSpace": self.cs_name, "GroupID": group_id, 
                  "GroupName": groupname, "HostName": hostname, "svcname": svcname } 
      self.db.sync(options = option)

   def tearDown(self):
      if self.should_clean_env():
         testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)

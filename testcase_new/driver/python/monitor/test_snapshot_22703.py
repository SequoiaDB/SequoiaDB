 # -*- coding: utf-8 -*-

# @testlink   seqDB-22703
# @interface  pysequoiadb.client.client.get_snapshot( self, snap_type, kwargs ) && snap_type = SBD_SNAP_INDEXSTATS
# @author     Zixian Yan 2020-08-31

from lib import testlib
from pysequoiadb.error import (SDBBaseError, SDBEndOfCursor)

SDB_SNAP_INDEXSTATS = 21

class Snapshot_22703(testlib.SdbTestBase):

   def setUp(self):
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      if testlib.is_standalone():
          self.group_name = "";
      else:
          groups = testlib.get_data_groups();
          self.group_name = groups[0]["GroupName"]

      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name, options = {"Group": self.group_name})

   def test_22703(self):
       record = [ {"a": 1}, {"a": 2} ]
       for item in record:
           self.cl.insert(item)

       index_def  = { "a": 1 }
       index_name = "index_22703"
       collection_name = self.cs_name + "." + self.cl_name

       self.cl.create_index_with_option( index_def, index_name, option = None)
       self.db.analyze( options = { "Collection": collection_name } )
       if testlib.is_standalone():
           snapshot_conditon = {"Index": index_name }
       else:
           master_node = self.db.get_replica_group_by_name(self.group_name).get_master()
           str_master_node =  master_node.get_hostname() + ":"+ master_node.get_servicename()
           snapshot_conditon = {"Index": index_name, "NodeName": str_master_node }


       detail = self.get_snapshot_result( SDB_SNAP_INDEXSTATS, condition = snapshot_conditon )
       result = self.get_result_from_snapshot( detail )
       expectation = { "Collection": collection_name, "Index": index_name, "GroupName": self.group_name, "MinValue": {"a":1}, "MaxValue":{"a":2} }
       self.assertEqual( result, expectation )

   def get_result_from_snapshot( self, info ):
       result = {}

       if testlib.is_standalone():
           detail = info[0]
           result["Collection"] = detail["Collection"]
           result["Index"]      = detail["Index"]
           result["GroupName"]  = detail["GroupName"]
           result["MinValue"]   = detail["MinValue"]
           result["MaxValue"]   = detail["MaxValue"]
       else:
           detail     = info[0]
           stat_info  = detail["StatInfo"]
           group_info = stat_info[0]["Group"]

           result["Collection"] = detail["Collection"]
           result["Index"]      = detail["Index"]
           result["GroupName"]  = stat_info[0]["GroupName"]
           result["MinValue"]   = group_info[0]["MinValue"]
           result["MaxValue"]   = group_info[0]["MaxValue"]

       return result

   def get_snapshot_result(self, snap_type, **kwargs):
      result = []
      cursor = self.db.get_snapshot(snap_type, **kwargs)
      while True:
         try:
            rc = cursor.next()
            result.append(rc)
         except SDBEndOfCursor:
            break
      cursor.close()
      return result

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)

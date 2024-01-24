# @decription: create cs and drop cs;
# @testlink:   seqDB-12442
# @interface:  create_collection_space(cs_name)
#              list_collection_spaces()
#              drop_collection_space(cs_name)
# @author:     zhaoyu 2017-8-24

from pysequoiadb.error import (SDBBaseError, SDBEndOfCursor)
from lib import testlib

class TestMeteData12442(testlib.SdbTestBase):
   def test_metedata_12442(self):
      #create cs and cl
      self.cs_name = "cs_12442"
      cl_name = "cl_12442"
      try:
         self.db.drop_collection_space(self.cs_name)
      except SDBBaseError as e:
         if -34 != e.code:
            self.fail("drop_cs_fail,detail:" + str(e))
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cs.create_collection(cl_name)
      
      #check cs exists or not
      self.check_list_collection_spaces(self.cs_name, True)
      
      #check cs options
      cs_options = {"PageSize": 65536, "LobPageSize": 262144}
      self.check_cs_snapshot_5(self.cs_name, cs_options)
      
      #drop cs and check exists or not
      self.db.drop_collection_space(self.cs_name)
      self.check_list_collection_spaces(self.cs_name, False)
             
   def tearDown(self):
      if self.should_clean_env():
         try:
            self.db.drop_collection_space(self.cs_name)
         except SDBBaseError as e:
            if -34 != e.code:
               self.fail("tear_down_fail,detail:" + str(e))

   def check_cs_snapshot_5(self, cs_name, options):
      cursor = self.db.get_snapshot(5, condition={"Name": cs_name})
      while True:
         try:
            record = cursor.next()
            self.assertDictContainsSubset(options, record)
         except SDBEndOfCursor:
            break
      cursor.close()
            
   def check_list_collection_spaces(self, expect_cs_name, cs_exists):
      cursor = self.db.list_collection_spaces()
      cs_names = []
      while True:
         try:
            record = cursor.next()
            cs_names.append(record['Name'])
         except SDBEndOfCursor:
            break
      cursor.close()
      if expect_cs_name in cs_names:
         self.assertTrue(cs_exists)
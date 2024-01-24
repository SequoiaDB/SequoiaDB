# @decription: create cs and drop cs
# @testlink:   seqDB-12444
# @interface:  create_collection_space(cs_name,options):options set dict,key set PageSize/LobPageSize
#              list_collection_spaces()
#              drop_collection_space(cs_name)
# @author:     zhaoyu 2017-8-24

from pysequoiadb.error import (SDBBaseError, SDBEndOfCursor)
from lib import testlib

class TestMeteData12444(testlib.SdbTestBase):
   def test_metedata_12444(self):
      self.list = [["cs_12444_1", 0, 524288, 65536, 524288],
                   ["cs_12444_2", 4096, 262144, 4096, 262144],
                   ["cs_12444_3", 8192, 131072, 8192, 131072],
                   ["cs_12444_4", 16384, 32768, 16384, 32768],
                   ["cs_12444_5", 32768, 65536, 32768, 65536],
                   ["cs_12444_6", 65536, 16384, 65536, 16384],
                   ["cs_12444_7", 0, 8192, 65536, 8192],
                   ["cs_12444_8", 0, 4096, 65536, 4096],
                   ["cs_12444_9", 0, 0, 65536, 262144]
                   ]
                  
      for index in range(len(self.list)):
         self.cs_name = self.list[index][0]
         try:
            self.db.drop_collection_space(self.cs_name)
         except SDBBaseError as e:
            if -34 != e.code:
               self.fail("drop_cs_fail,detail:" + str(e))
         page_size = self.list[index][1]
         lob_page_size = self.list[index][2]
         cs_options = {"PageSize": page_size, "LobPageSize": lob_page_size}
         expect_page_size = self.list[index][3]
         expect_lob_page_size = self.list[index][4]
         expect_cs_options = {"PageSize": expect_page_size, "LobPageSize": expect_lob_page_size}
         self.check_create_cs(self.cs_name, cs_options, expect_cs_options)
   
   def tearDown(self):
      if self.should_clean_env():
         try:
            for index in range(len(self.list)):
               self.db.drop_collection_space(self.cs_name)
         except SDBBaseError as e:
            if -34 != e.code:
               self.fail("tear_down_fail,detail:" + str(e))
           
   def check_create_cs(self, cs_name, cs_options, expect_cs_options):
      #create cs and cl
      cl_name = "cl"
      self.cs = self.db.create_collection_space(cs_name, cs_options)
      self.cs.create_collection(cl_name)
      
      #check cs exists or not
      self.check_list_collection_spaces(cs_name, True)
      
      #check cs options
      self.check_cs_snapshot_5(self.cs_name, expect_cs_options)
      
      #drop cs and check exists or not      
      self.db.drop_collection_space(self.cs_name)
      self.check_list_collection_spaces(cs_name, False)
      
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
      flag = False
      cs_names = []
      while True:
         try:
            record = cursor.next()
            cs_names.append(record['Name'])
         except SDBEndOfCursor:
            break
      cursor.close()
      if expect_cs_name in cs_names:
         flag = True
         self.assertEqual(flag, cs_exists)

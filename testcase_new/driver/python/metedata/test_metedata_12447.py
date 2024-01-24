# @decription: get/list cs and cl
# @testlink:   seqDB-12447
# @interface:  get_collection_space(cs_name)
#              get_collection(cl_full_name)/get_collection(cl_name)
#              get_collection_space_name()
#              get_collection_name()
#              get_cs_name()
#              get_full_name()
#              format support . or [] to get cs/cl
# @author:     zhaoyu 2017-8-30

from pysequoiadb.error import SDBBaseError
from lib import testlib

class TestMeteData12447(testlib.SdbTestBase):
   def test_metedata_12447(self):
      #create cs
      self.cs_name = "cs_12447"
      cl_name = "cl_12447"
      try:
         self.db.drop_collection_space(self.cs_name)
      except SDBBaseError as e:
         if -34 != e.code:
            self.fail("drop_cs_fail,detail:" + str(e))
      self.db.create_collection_space( self.cs_name )
      
      #get cs and create cl
      cs = self.db.get_collection_space(self.cs_name)
      cs.create_collection(cl_name)
      
      #get cl full name and check cl name
      cl1 = self.db.get_collection(self.cs_name + "." + cl_name)
      actual_cl_name1 = cl1.get_collection_name()
      self.assertEqual(actual_cl_name1, cl_name)
      
      #get cl from cl name and check cl name
      cl2 = cs.get_collection(cl_name)
      actual_cl_name2 = cl2.get_collection_name()
      self.assertEqual(actual_cl_name2, cl_name)
      
      #get cs name
      actual_cs_name1 = cs.get_collection_space_name()
      self.assertEqual(actual_cs_name1, self.cs_name)
      
      actual_cs_name2 = cl1.get_cs_name()
      self.assertEqual(actual_cs_name2, self.cs_name)
      
      actual_cs_name3 = cl2.get_cs_name()
      self.assertEqual(actual_cs_name3, self.cs_name)
      
      #get cl full name
      actual_cl_full_name1 = cl1.get_full_name()
      self.assertEqual(actual_cl_full_name1, self.cs_name + "." + cl_name)
      
      actual_cl_full_name2 = cl2.get_full_name()
      self.assertEqual(actual_cl_full_name1, self.cs_name + "." + cl_name)
      
      #check . or [] get cs/cl name
      actual_cs_name4 = self.db.cs_12447.get_collection_space_name()
      self.assertEqual(actual_cs_name4, self.cs_name)
      
      actual_cs_name5 = self.db[self.cs_name].get_collection_space_name()
      self.assertEqual(actual_cs_name5, self.cs_name)
      
      actual_cl_name3 = self.db.cs_12447.cl_12447.get_collection_name()
      self.assertEqual(actual_cl_name3, cl_name)
      
      actual_cl_name4 = self.db[self.cs_name][cl_name].get_collection_name()
      self.assertEqual(actual_cl_name4, cl_name)
      
   def tearDown(self):
      if self.should_clean_env():
         try:
            self.db.drop_collection_space(self.cs_name)
         except SDBBaseError as e:
            if -34 != e.code:
               self.fail("tear_down_fail,detail:" + str(e))
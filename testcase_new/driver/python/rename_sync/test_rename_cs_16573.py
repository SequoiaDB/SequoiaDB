# @decription rename collection_space and metadata operation
# @testlink   seqDB-16573
# @interface  rename_collection_space	( self, old_name, new_name, options = None )		
# @author     yinzhen 2018-12-06

from lib import testlib
from lib import sdbconfig
from pysequoiadb.error import (SDBBaseError, SDBEndOfCursor)

class TestRenameCS16573(testlib.SdbTestBase):
   def setUp(self):
      # create cs 
      self.test_cs_name = "test1_16573"
      self.test_cs_newname = "test2_16573"
      testlib.drop_cs(self.db, self.test_cs_name, ignore_not_exist=True)
      testlib.drop_cs(self.db, self.test_cs_newname, ignore_not_exist=True)      
		 
   def test_rename_cs_16573(self):
      #rename collection space
      self.cs = self.db.create_collection_space(self.test_cs_name)
      self.assertTrue(self.is_collection_space_exist(self.test_cs_name))
      
      #rename collection space
      self.db.rename_collection_space(self.test_cs_name, self.test_cs_newname)
      self.assertTrue(self.is_collection_space_exist(self.test_cs_newname))
      self.assertFalse(self.is_collection_space_exist(self.test_cs_name))
      
      #create collection
      self.cs = self.db.get_collection_space(self.test_cs_newname)
      self.cs.create_collection("test_cl")
      self.assertTrue(self.is_collection_exist("test_cl"))
      
      #drop collection
      self.cs.drop_collection("test_cl")
      self.assertFalse(self.is_collection_exist("test_cl"))
      
      #rename not exist collection space
      try:
         self.db.rename_collection_space(self.test_cs_name, "test3_16573")
      except SDBBaseError as e:
         self.assertEqual(-34, e.code, e.detail)
      self.assertFalse(self.is_collection_space_exist("test3_16573"))
      
   def tearDown(self):
      self.db.drop_collection_space(self.test_cs_newname)   
      self.db.disconnect()
      
   def is_collection_space_exist(self, collection_space_name):
      try:
         cs_name = self.db.get_collection_space(collection_space_name).get_collection_space_name()
         if (cs_name == collection_space_name):
            return True
      except SDBBaseError as e:
         self.assertEqual(-34, e.code, e.detail)
         return False
      
   def is_collection_exist(self, collection_name):
      try:
         cl_name = self.cs.get_collection(collection_name).get_collection_name()
         if (cl_name == collection_name):
            return True
      except SDBBaseError as e:
         self.assertEqual(-23, e.code, e.detail)
         return False         
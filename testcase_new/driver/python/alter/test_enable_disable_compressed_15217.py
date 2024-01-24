# @decription: test enable compression, disable compression
# @testlink:   seqDB-15217
# @interface:  enable_compression(self, options)
# @interface:  disable_compression(self)
# @author:     liuxiaoxuan 2018-04-25

import unittest
from pysequoiadb.error import (SDBBaseError, SDBEndOfCursor)
from lib import testlib

class TestEnableDisableCompressed15217(testlib.SdbTestBase):
   def setUp(self):
      # skip standalone
      if testlib.is_standalone():
         self.skipTest('run mode is standalone')
      # create cs cl      
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)

   def test_enable_disable_compressed_15217(self):
      
      # alter cl, enable compression
      enable_opts = {'CompressionType' : 'lzw'}
      self.cl.enable_compression(options = enable_opts)
      
      # check collection
      expect_attributes = [{"CompressionType": 1, "CompressionTypeDesc": "lzw"}]
      self.check_enable_compress_attributes(expect_attributes, condition = {'Name': self.cs_name + '.' + self.cl_name})
      
      # disable compression
      self.cl.disable_compression()
      
      # check collection
      self.check_disable_compress_attributes(condition = {'Name': self.cs_name + '.' + self.cl_name})
		
   def tearDown(self):
      self.db.drop_collection_space(self.cs_name)
      self.db.disconnect()

   def get_collection_attributes(self, **kwargs):
      act_attributes = list()
      cursor = self.db.get_snapshot(8, **kwargs)
      while(True):
         try:
            act_attributes.append(cursor.next())
         except SDBEndOfCursor: 
            break
      return act_attributes
      
   def check_enable_compress_attributes(self, expect_attributes, **kwargs):
      act_attributes = self.get_collection_attributes(**kwargs)
      
      # compare results
      self.assertEqual(len(act_attributes), len(expect_attributes))
      self.assertEqual(len(act_attributes), 1)
      self.assertEqual(act_attributes[0]['CompressionType'], expect_attributes[0]['CompressionType'])
      self.assertEqual(act_attributes[0]['CompressionTypeDesc'], expect_attributes[0]['CompressionTypeDesc'])
  
   def check_disable_compress_attributes(self, **kwargs):
      act_attributes = self.get_collection_attributes(**kwargs)
      
      # check results
      self.assertEqual(len(act_attributes), 1)
      self.assertNotIn('CompressionType', list(act_attributes[0].keys()))
      self.assertNotIn('CompressionTypeDesc', list(act_attributes[0].keys()))
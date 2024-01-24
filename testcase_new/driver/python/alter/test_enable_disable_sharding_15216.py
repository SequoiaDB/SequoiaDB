# @decription: test enable sharding, disable sharding
# @testlink:   seqDB-15216
# @interface:  enable_sharding(self, options)
# @interface:  disable_sharding(self)
# @author:     liuxiaoxuan 2018-04-25

import unittest
from pysequoiadb.error import (SDBBaseError, SDBEndOfCursor)
from lib import testlib

class TestEnableDisableSharding15216(testlib.SdbTestBase):
   def setUp(self):
      # skip standalone
      if testlib.is_standalone():
         self.skipTest('run mode is standalone')
      
      # create cs cl
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)      
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)

   def test_enable_disable_sharding_15216(self):
      
      # alter cl, enable sharding
      enable_opts = {'ShardingKey': {'a':1}, 'ShardingType': 'hash', 'Partition': 128, 'EnsureShardingIndex': False}
      self.cl.enable_sharding(options = enable_opts)
      
      # check collection
      expect_attributes = [enable_opts]
      self.check_enable_sharding_attributes(expect_attributes, condition = {'Name' : self.cs_name + '.' + self.cl_name})
      
      # disable sharding
      self.cl.disable_sharding()
      
      # check collection
      self.check_disable_sharding_attributes(condition = {'Name' : self.cs_name + '.' + self.cl_name})
		
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
      
   def check_enable_sharding_attributes(self, expect_attributes, **kwargs):
      act_attributes = self.get_collection_attributes(**kwargs)
      
      # compare results
      self.assertEqual(len(act_attributes), len(expect_attributes))
      self.assertEqual(len(act_attributes), 1)
      self.assertEqual(act_attributes[0]['ShardingKey'], expect_attributes[0]['ShardingKey'])
      self.assertEqual(act_attributes[0]['ShardingType'], expect_attributes[0]['ShardingType'])
      self.assertEqual(act_attributes[0]['Partition'], expect_attributes[0]['Partition'])
      self.assertEqual(act_attributes[0]['EnsureShardingIndex'], expect_attributes[0]['EnsureShardingIndex'])
           
   def check_disable_sharding_attributes(self, **kwargs):
      act_attributes = self.get_collection_attributes(**kwargs)
      
      # check results
      self.assertEqual(len(act_attributes), 1)
      self.assertNotIn('ShardingKey', list(act_attributes[0].keys()))
      self.assertNotIn('ShardingType', list(act_attributes[0].keys()))
      self.assertNotIn('Partition', list(act_attributes[0].keys()))
      self.assertNotIn('EnsureShardingIndex', list(act_attributes[0].keys()))
# -*- coding: utf-8 -*-
# @decription: seqDB-24535:dropCS指定options
# @author:     liuli
# @createTime: 2021.10.29

import unittest
from lib import testlib
from pysequoiadb.error import SDBBaseError

cs_neme = "cs_24535"
class TestDropCS24535(testlib.SdbTestBase):
   def setUp(self):
      testlib.drop_cs(self.db, cs_neme, ignore_not_exist=True)
      
   def test_dropCS_24535(self):
      cl_name = "cl_24535"
      # cs下存在cl，指定option中EnsureEmpty为false
      cs1 = self.db.create_collection_space(cs_neme)
      cs1.create_collection(cl_name)
      self.db.drop_collection_space(cs_neme, {'EnsureEmpty':False})
      try:
         self.db.get_collection_space(cs_neme)
         self.fail('should error but success')
      except SDBBaseError as e:
         if -34 != e.code:
            raise e

      # cs下不存在cl，指定option中EnsureEmpty为false
      self.db.create_collection_space(cs_neme)
      self.db.drop_collection_space(cs_neme, {'EnsureEmpty':False})
      try:
         self.db.get_collection_space(cs_neme)
         self.fail('should error but success')
      except SDBBaseError as e:
         if -34 != e.code:
            raise e

      # cs下不存在cl，指定option中EnsureEmpty为true
      self.db.create_collection_space(cs_neme)
      self.db.drop_collection_space(cs_neme, {'EnsureEmpty':True})
      try:
         self.db.get_collection_space(cs_neme)
         self.fail('should error but success')
      except SDBBaseError as e:
         if -34 != e.code:
            raise e

      # cs下存在cl，指定option中EnsureEmpty为true
      cs2 = self.db.create_collection_space(cs_neme)
      cs2.create_collection(cl_name)
      try:
         self.db.drop_collection_space(cs_neme, {'EnsureEmpty': True})
         self.fail('should error but success')
      except SDBBaseError as e:
         if -275 != e.code:
            raise e
      self.db.get_collection_space(cs_neme)
     
   def tearDown(self):
      testlib.drop_cs(self.db, cs_neme, ignore_not_exist=True)
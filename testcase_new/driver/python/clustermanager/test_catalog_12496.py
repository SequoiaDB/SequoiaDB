# @decription: catalog
# @testlink:   seqDB-12496
# @interface:  is_catalog
#              get_cata_replica_group(self)
# @author:     zhaoyu 2017-9-8

import unittest
from lib import testlib
from lib import sdbconfig

class TestCatalog12496(testlib.SdbTestBase):
   def setUp(self):
      # check standalone
      if testlib.is_standalone():
         self.skipTest('run mode is standalone')
      
   def test_catalog_12496(self):
      # get catalog rg and check is_catalog
      catalog_rg = self.db.get_cata_replica_group()
      is_catalog = catalog_rg.is_catalog()
      self.assertTrue(is_catalog)
   
   def tearDown(self):
      pass
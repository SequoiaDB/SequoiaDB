# -*- coding: utf-8 -*-
# @decription: seqDB-24876:驱动支持cs.getDomainName()
# @author:     liuli
# @createTime: 2021.12.22

import unittest
from lib import testlib
from pysequoiadb.error import SDBBaseError

cs_neme = "cs_24876"
domain_name1 = "domain_24876_1"
domain_name2 = "domain_24876_2"
class TestGetDomainName24876(testlib.SdbTestBase):
   def setUp(self):
      if testlib.is_standalone():
         self.skipTest('current environment is standalone')
      testlib.drop_cs(self.db, cs_neme, ignore_not_exist=True)
      self.drop_domain(self.db, domain_name1, ignore_not_exist=True)
      self.drop_domain(self.db, domain_name2, ignore_not_exist=True)
      
   def test_dropCS_24876(self):
      cl_name = "cl_24876"

      data_groups = testlib.get_data_groups()
      group = data_groups[0]['GroupName']
      options = {'Groups': [group]}
      self.db.create_domain(domain_name1, options)
      self.db.create_domain(domain_name2, options)

      cs_options = {"Domain": domain_name1}
      dbcs = self.db.create_collection_space(cs_neme, cs_options)
      dbcs.create_collection(cl_name)

      act_domain = dbcs.get_domain_name()
      if act_domain != domain_name1:
         raise Exception("expect domain is ", domain_name1, ", actual domain is ", act_domain)

      set_option = {"Domain": domain_name2}
      dbcs.set_domain(set_option)
      act_domain = dbcs.get_domain_name()
      if act_domain != domain_name2:
         raise Exception("expect domain is ", domain_name2, ", actual domain is ", act_domain)

      dbcs.remove_domain()
      exp_domain = ""
      act_domain = dbcs.get_domain_name()
      if act_domain != exp_domain:
         raise Exception("expect domain is null, actual domain is ", act_domain)

   def tearDown(self):
      testlib.drop_cs(self.db, cs_neme, ignore_not_exist=True)
      self.drop_domain(self.db, domain_name1, ignore_not_exist=True)
      self.drop_domain(self.db, domain_name2, ignore_not_exist=True)

   def drop_domain(self, db, domain_name, ignore_not_exist=False):
      try:
         db.drop_domain(domain_name)
      except SDBBaseError as e:
         if ignore_not_exist == True:
            if -214 != e.code:
               raise e
         else:
            raise e
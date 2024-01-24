# @testlink:   seqDB-13483
# @interface:  list_domains(kwargs) 
#              get_domain(domain_name)
#              domain.list_collection_spaces() 
#              domain.list_collections()
#              domain.name
# @author:     liuxiaoxuan 2017-11-21

import unittest
from lib import testlib
from lib import sdbconfig
from pysequoiadb import client
from pysequoiadb.error import SDBBaseError

domain_name = "domain_13483"
class TestMetedata13482(testlib.SdbTestBase):
   def setUp(self):
      # check standalone
      if testlib.is_standalone():
         self.skipTest('current environment is standalone')
      self.domain_name1 = domain_name + "_1"
      self.domain_name2 = domain_name + "_2"
      self.cs_name1 = self.cs_name + "_1"
      self.cs_name2 = self.cs_name + "_2"
      testlib.drop_cs(self.db, self.cs_name1, ignore_not_exist=True)
      testlib.drop_cs(self.db, self.cs_name2, ignore_not_exist=True)
      
   def test_metedata_13483(self):
      # get data groups
      data_groups = testlib.get_data_groups()
      group1 = data_groups[0]['GroupName']
      
      # create domains 
      self.db.create_domain(self.domain_name1, {'Groups' : [group1]})
      self.db.create_domain(self.domain_name2, {'Groups' : [group1], 'AutoSplit' : True})
      
      # create CSs and CLs
      cl_name1 = self.cl_name + "_1"
      cl_name2 = self.cl_name + "_2"
      
      cs1 = self.db.create_collection_space(self.cs_name1, 
                                            options = {'Domain' : self.domain_name1})
      cs1.create_collection(cl_name1)
      cs1.create_collection(cl_name2)
      
      cs2 = self.db.create_collection_space(self.cs_name2)
      cs2.create_collection(cl_name1)
      cs2.create_collection(cl_name2)
      
      # list all domains
      expect_result = [self.domain_name1,self.domain_name2]
      self.check_list_domains(expect_result)
      
      # check domain has cs and cl
      domain = self.db.get_domain(self.domain_name1)
      act_domain_name = domain.name 
      self.assertEqual(act_domain_name, self.domain_name1)
      
      # check cs
      cur1 = domain.list_collection_spaces()
      list_cs = testlib.get_all_records(cur1)
      self.assertEqual(1, len(list_cs))
      self.assertEqual(self.cs_name1, list_cs[0]['Name'])
      
      # check cl
      cur2 = domain.list_collections()
      list_cl = testlib.get_all_records(cur2)
      expect_cls = [{'Name' : self.cs_name1 + "." + cl_name1},
                    {'Name' : self.cs_name1 + "." + cl_name2}]
      self.assertEqual(2, len(list_cl))
      self.assertListEqualUnordered(expect_cls, list_cl)
      
      # check domain not exsit cs 
      domain = self.db.get_domain(self.domain_name2)
      act_domain_name = domain.name 
      self.assertEqual(act_domain_name, self.domain_name2)
      
      cur1 = domain.list_collection_spaces()
      list_cs = testlib.get_all_records(cur1)
      self.assertEqual(0, len(list_cs))
      
      cur2 = domain.list_collections()
      list_cl = testlib.get_all_records(cur2)
      self.assertEqual(0, len(list_cl))

      # checkout domain again
      domain = self.db.get_domain(self.domain_name1)
      act_domain_name = domain.name 
      self.assertEqual(act_domain_name, self.domain_name1)
     
   def tearDown(self):
      # drop cs
      testlib.drop_cs(self.db, self.cs_name1, ignore_not_exist=True)
      testlib.drop_cs(self.db, self.cs_name2, ignore_not_exist=True)
      
      # drop domain
      msg = "tear down fail"
      self.drop_domain(self.domain_name1, msg)
      self.drop_domain(self.domain_name2, msg)
            
   def check_list_domains(self, expect_result, **kwargs):
      cur = self.db.list_domains(**kwargs)
      act_result = list()
      while True:
         try:
            rec = cur.next()
            act_result.append(rec['Name'])
         except SDBBaseError as e:
            break
      cur.close()
      for x in expect_result:
         self.assertIn(x, act_result, x + " not in " + str(act_result))
   
   def drop_domain(self, domain_name, msg):
      try:
         self.db.drop_domain(domain_name)
      except SDBBaseError as e:
         if e.code != -214:
            self.fail(msg + str(e))
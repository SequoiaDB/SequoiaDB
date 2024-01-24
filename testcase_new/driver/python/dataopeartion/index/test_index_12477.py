# @decription: create/drop/query id index
# @testlink:   seqDB-12477
# @interface:  create_id_index(self,options)
#              drop_id_index(self)
#              get_index_info(self,idx_name)
#              is_index_exist(self,idx_name)
# @author:     liuxiaoxuan 2017-8-30

from lib import testlib
from pysequoiadb.error import (SDBBaseError, SDBEndOfCursor)

insert_nums = 100


class TestIdIndex12477(testlib.SdbTestBase):
   def setUp(self):
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)
      self.insert_datas()

   def test_id_index_12477(self):
      # create index without option
      option = None
      self.create_id_index(option)
      self.assertTrue(self.cl.is_index_exist('$id'))
      # drop index
      self.drop_id_index()
      # check index
      self.assertFalse(self.cl.is_index_exist('$id'))
      # check drop result
      is_success = True
      self.check_update_result(not is_success)
      # create index with option
      option = {'SortBufferSize': 64}
      self.create_id_index(option)
      # check index
      expect_idx_name = '$id'
      self.assertTrue(self.cl.is_index_exist(expect_idx_name))
      self.check_id_index(expect_idx_name)
      self.check_update_result(is_success)

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)

   def insert_datas(self):
      doc = []
      for i in range(0, insert_nums):
         doc.append({"a": i, "b": "test" + str(i)})
      try:
         flags = 0
         self.cl.bulk_insert(flags, doc)
      except SDBBaseError as e:
         self.fail('insert fail: ' + str(e))

   def create_id_index(self, opt):
      try:
         if opt == None:
            self.cl.create_id_index()
         else:
            self.cl.create_id_index(options=opt)
      except SDBBaseError as e:
         if not -247 == e.code:
            self.fail('create index fail: ' + str(e))

   def drop_id_index(self):
      try:
         self.cl.drop_id_index()
      except SDBBaseError as e:
         if not -47 == e.code:		
            self.fail('drop index fail: ' + str(e))

   def check_update_result(self, is_success):
      try:
         # check update
         rule = {'$set': {'b': "update"}}
         cond = {'a': {'$gt': 10}}
         self.cl.update(rule, condition=cond)
         # update fail without id index
         if not is_success:
            self.fail('NEED UPDATE FAIL')
      except SDBBaseError as e:
         if is_success:
            self.fail('update fail error: ' + str(e))
         else:
            self.assertEqual(-279, e.code, 'update fail error: ' + e.detail)

   def check_id_index(self, expect_name):
      act_name = ''
      try:
         rec = self.cl.get_index_info(expect_name)
         act_name = rec['IndexDef']['name']
         self.assertEqual(expect_name, act_name, expect_name + ' not equal to ' + act_name)
      except SDBBaseError as e:
         self.fail('check id index fail: ' + str(e))

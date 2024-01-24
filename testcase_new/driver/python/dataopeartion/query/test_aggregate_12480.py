# @decription: test aggregate
# @testlink:   seqDB-12480
# @interface:  aggregate(self,aggregate_options)
# @author:     liuxiaoxuan 2017-8-30

from bson.son import SON
from lib import testlib
from pysequoiadb.error import (SDBError)

insert_nums = 10
class TestAggregate12480(testlib.SdbTestBase):
   def setUp(self):
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist = True)
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)
      self.insert_datas()

   def test_aggregate_12480(self):
      match = SON({'$match': {'name': {'$exists': 1}}})
      group = SON({'$group': {'_id': '$major', 'avg_age': {'$avg': '$age'}, 'major': {'$first': '$major'}}})
      sort = SON({'$sort': {'avg_age': 1}})
      skip = {'$skip': 1}
      limit = {'$limit': 2}

      list_aggr_options = [match, group, sort, skip, limit]
      tuple_aggr_options = (match, group, sort, skip, limit)

      list_cursor = self.cl.aggregate(list_aggr_options)
      tuple_cursor = self.cl.aggregate(list_aggr_options)

      list_actResult = testlib.get_all_records(list_cursor)
      tuple_actResult = testlib.get_all_records(tuple_cursor)

      expectResult = [{'avg_age': 20.0, 'major': 'major5'}, \
                      {'avg_age': 21.0, 'major': 'major1'}]

      self.assertListEqualUnordered(expectResult, list_actResult)
      self.assertListEqualUnordered(expectResult, tuple_actResult)

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)

   def insert_datas(self):
      doc = []
      for i in range(0, insert_nums):
         doc.append({"_id": i, "name": "test" + str(i), "major": "major" + str(i % 10), "age": 20 + i % 5})
      try:
         flags = 0
         self.cl.bulk_insert(flags, doc)
      except SDBBaseError as e:
         self.fail('insert fail: ' + str(e))

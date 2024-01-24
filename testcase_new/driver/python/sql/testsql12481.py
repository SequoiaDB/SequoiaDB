# -- coding: utf-8 --
# @decription: test sql
# @testlink:   12481
# @author:     LaoJingTang
from lib import testlib


class SdbTestSql12481(testlib.SdbTestBase):
   def setUp(self):
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)

   def tearDown(self):
      pass

   def test_select(self):
      self.cl.bulk_insert(0, [{"a": 1} for i in range(10)])
      db = self.db
      # select
      sql = "select a from " + self.cl_name_qualified
      cur = db.exec_sql(sql)
      e = [{"a": 1} for i in range(10)]
      self.assertListEqualUnordered(e, testlib.get_all_records_noid(cur))

      # select avg
      sql = "select avg(a) as avg from " + self.cl_name_qualified
      cur = db.exec_sql(sql)
      a = testlib.get_all_records_noid(cur)
      e = [{"avg": 1}]
      self.assertListEqualUnordered(e, a)

      # select group by
      sql = "select count(a) as a from " + self.cl_name_qualified + " group by a"
      cur = db.exec_sql(sql)
      a = testlib.get_all_records_noid(cur)
      e = [{"a": 10}]
      self.assertListEqualUnordered(e, a)

      # select join
      sql = "select t1.a,t2.a from " + self.cl_name_qualified + " as t1 inner join " + self.cl_name_qualified + " as t2 on t1.a =t2.a"
      cur = db.exec_sql(sql)
      a = testlib.get_all_records_noid(cur)
      e = [{"a": 1, "a": 1} for i in range(100)]
      self.assertListEqualUnordered(e, a)
      self.db.drop_collection_space(self.cs_name)

   def test_sql(self):
      self.db.drop_collection_space(self.cs_name)
      # create CS
      db = self.db
      sql = "create collectionspace " + self.cs_name
      db.exec_update(sql)

      # list CS
      sql = "list collectionspaces"
      cur = db.exec_sql(sql)
      r = testlib.get_all_records_noid(cur=cur)
      self.assertIn({"Name": self.cs_name}, r)

      # create CL
      sql = "create collection " + self.cs_name + "." + self.cl_name
      db.exec_update(sql)

      # list CL
      sql = "list collections"
      cur = db.exec_sql(sql)
      r = testlib.get_all_records_noid(cur)
      self.assertIn({"Name": self.cl_name_qualified}, r)

      # insert
      for i in range(10):
         sql = "insert into " + self.cl_name_qualified + "(a) values(" + str(i) + ")"
         db.exec_update(sql)
      e = [{"a": i} for i in range(10)]
      self.cl = self.db.get_collection(self.cl_name_qualified)
      a = testlib.get_all_records_noid(self.cl.query())
      self.assertListEqualUnordered(e, a)

      # update
      sql = "update " + self.cl_name_qualified + " set a=0"
      db.exec_update(sql)
      e = [{"a": 0} for i in range(10)]
      a = testlib.get_all_records_noid(self.cl.query())
      self.assertListEqualUnordered(e, a)

      # delete
      sql = "delete from " + self.cl_name_qualified
      db.exec_update(sql)
      e = []
      a = testlib.get_all_records_noid(self.cl.query())
      self.assertListEqualUnordered(e, a)

      # dropCs
      sql = "drop collectionspace " + self.cs_name
      db.exec_update(sql)
      cur = db.list_collection_spaces()
      r = testlib.get_all_records_noid(cur)
      self.assertNotIn({"Nam": self.cs_name}, r)

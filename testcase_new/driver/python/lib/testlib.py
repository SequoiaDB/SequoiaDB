# -- coding: utf-8 --
import unittest
from copy import copy
from datetime import datetime

from pysequoiadb import SDBError
from pysequoiadb import client
from pysequoiadb.error import SDBBaseError

from lib import sdbconfig


class SdbTestBase(unittest.TestCase):
   def __init__(self, methodName='runTest'):
      unittest.TestCase.__init__(self, methodName=methodName)
      self.__groups = []
      self.__data_groups = []

   @classmethod
   def setUpClass(cls):
      print(cls.__name__ + " setup: " + str(datetime.now()))
      cls.cs_name = cls.__class__.__name__ + "_cs"
      cls.cl_name = cls.__class__.__name__ + "_cl"
      cls.cl_name_qualified = cls.cs_name + "." + cls.cl_name
      cls.db = default_db()

   @classmethod
   def tearDownClass(cls):
      cls.db.disconnect()
      print(cls.__name__ + " teardown: " + str(datetime.now()))

   def assertListEqualUnordered(self, expected, actual, msg=None):
      """
      判断两个数组是否相等，并不要求两个数组具有相同的顺序
      :param expected: list expected
      :param actual: list actual
      """
      if msg != None:
         msg = "\n" + msg + "\nexpected: " + str(expected) + "\nactual: " + str(actual)
      else:
         msg = "\nexpected: " + str(expected) + "\nactual: " + str(actual)

      unittest.TestCase.assertEqual(self, len(expected), len(actual), msg=msg)

      # 使用result进行校验，可以避免[{a:1},{a:1},{a:2}]，[{a:1},{a:2},{a:2}]被误判为相等的问题
      result = list(expected)
      for x in actual:
         unittest.TestCase.assertIn(self, x, result, msg)
         result.remove(x)

      result = list(actual)
      for x in expected:
         unittest.TestCase.assertIn(self, x, result, msg)
         result.remove(x)

   def should_clean_env(self):
      if not isinstance(self, unittest.TestCase):
         raise TypeError("should_clear_env() arg must be unittest.TestCase")

      r = True
      try:
         r = self.__is_testcase_success()
      except BaseException:
         r = True
      if r:
         return True
      else:
         return not sdbconfig.sdb_config.break_on_failure

   def __is_testcase_success(self):
      """
      判断当前用例是否成功
      :param self:
      :return:
      """
      if hasattr(self, "_outcome"):
         # for python 3.6.2
         for x in self._outcome.errors:
            if x[1] != None:
               return False
         return True
      elif hasattr(self, "_outcomeForDoCleanups"):
         # for python 3.3.4
         return self._outcomeForDoCleanups.success
      elif hasattr(self, "_resultForDoCleanups"):
         # for python 2 unittest
         failures = self._resultForDoCleanups.failures
         errors = self._resultForDoCleanups.errors
         l = []
         l.extend(failures)
         l.extend(errors)
         for x in l:
            if isinstance(x, xmlrunner._TestInfo) and self == x.test_method:
               return False
         return True
      else:
         # can not judge this testcase success or failed ,so think it was success
         print("warn: can not judge this testcase success.")
         return True


__is_standalone_flag = None


def is_standalone():
   global __is_standalone_flag
   if __is_standalone_flag != None:
      return __is_standalone_flag
   else:
      try:
         db = default_db()
         db.list_replica_groups()
         __is_standalone_flag = False
         return False
      except SDBError as e:
         if e.code == -159:
            __is_standalone_flag = True
            return True
         else:
            raise e
      finally:
         if db != None:
            db.disconnect()


__groups = []
__data_groups = []


def get_groups():
   global __groups
   if __groups.__len__() > 0:
      return copy(__groups)
   else:
      try:
         db = default_db()
         cur = db.list_replica_groups()
         r = get_all_records_noid(cur)
         __groups.extend(r)
         return copy(__groups)
      finally:
         if db != None:
            db.disconnect()


def get_data_groups():
   global __data_groups
   global __groups
   if __data_groups.__len__() > 0:
      return copy(__data_groups)
   else:
      if len(__groups) == 0:
         get_groups()
      for x in __groups:
         if x["GroupName"] != "SYSCatalogGroup" and x["GroupName"] != "SYSCoord":
            __data_groups.append(x)
      return copy(__data_groups)


def get_data_group_num():
   if __data_groups.__len__() > 0:
      return __data_groups.__len__()
   else:
      get_data_groups()
      return __data_groups.__len__()


def default_db():
   return client(sdbconfig.sdb_config.host_name, sdbconfig.sdb_config.service)


def get_all_records(cur):
   """
   :param cur: 游标
   :return: 记录数组
   """
   items = list()
   while True:
      try:
         item = cur.next()
         items.append(item)
      except BaseException as e:
         break
   cur.close()
   return items


def get_all_records_noid(cur):
   items = list()
   while True:
      try:
         item = cur.next()
         if "_id" in item:
            item.pop('_id')
         items.append(item)
      except BaseException as e:
         break
   cur.close()
   return items


def drop_cs(db, cs_name, ignore_not_exist=False):
   try:
      db.drop_collection_space(cs_name)
   except SDBBaseError as e:
      if ignore_not_exist == True:
         if -34 != e.code:
            raise e
      else:
         raise e


def drop_cs_if_exist(db, cs_name):
   try:
      db.drop_collection_space(cs_name)
   except SDBBaseError as e:
      if -34 != e.code:
         raise e

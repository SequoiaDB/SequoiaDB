# -- coding: utf-8 --
# @decription: insert decimal data
# @testlink:   seqDB-9462/seqDB-9463/seqDB-9464/seqDB-9465/seqDB-9466
#              /seqDB-9467/seqDB-9468/seqDB-9469/seqDB-9470/seqDB-9471
#              /seqDB-9472/seqDB-9473/seqDB-9474/seqDB-10493
# @interface:  insert(record)
#              update(rule, kwargs)
#              delete(kwargs)
# @author:     zhaoyu 2017-9-7

from collections import OrderedDict

from bson.decimal import Decimal
from bson.json_util import dumps
from bson.json_util import loads
from bson.objectid import ObjectId
from pysequoiadb.error import (SDBBaseError)

from lib import testlib


class TestDecimal12459(testlib.SdbTestBase):
   def setUp(self):
      # create cs and cl
      self.cs_name = "cs_12459"
      self.cl_name = "cl_12459"
      try:
         self.db.drop_collection_space(self.cs_name)
      except SDBBaseError as e:
         if -34 != e.code:
            self.fail("drop_cs_fail,detail:" + str(e))
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name, {"ReplSize": 0})

   def test_decimal_12459(self):
      # seqDB-9462
      obj = Decimal(2147483647, 10, 0)
      doc = OrderedDict([("a", obj), ("_id", 1)])
      self.insert_decimal(self.cl, doc)
      self.check_result(self.cl, {"a": OrderedDict([("$type", 1), ("$et", 100)])}, obj, doc)
      self.cl.delete()

      # 2
      obj = Decimal(-2147483648, 10, 0)
      doc = OrderedDict([("a", obj), ("_id", 1)])
      self.insert_decimal(self.cl, doc)
      self.check_result(self.cl, {"a": OrderedDict([("$type", 1), ("$et", 100)])}, obj, doc)
      self.cl.delete()

      # 3
      obj = Decimal(0, 5, 2)
      doc = OrderedDict([("a", obj), ("_id", 1)])
      self.insert_decimal(self.cl, doc)
      self.check_result(self.cl, {"a": OrderedDict([("$type", 1), ("$et", 100)])}, obj, doc)
      self.cl.delete()

      # 4
      obj = Decimal(9223372036854775807, 19, 0)
      doc = OrderedDict([("a", obj), ("_id", 1)])
      self.insert_decimal(self.cl, doc)
      self.check_result(self.cl, {"a": OrderedDict([("$type", 1), ("$et", 100)])}, obj, doc)
      self.cl.delete()

      # 5
      obj = Decimal(-9223372036854775808, 19, 0)
      doc = OrderedDict([("a", obj), ("_id", 1)])
      self.insert_decimal(self.cl, doc)
      self.check_result(self.cl, {"a": OrderedDict([("$type", 1), ("$et", 100)])}, obj, doc)
      self.cl.delete()

      # 6
      obj = Decimal(1.7e308, 1000, 2)
      doc = OrderedDict([("a", obj), ("_id", 1)])
      self.insert_decimal(self.cl, doc)
      self.check_result(self.cl, {"a": OrderedDict([("$type", 1), ("$et", 100)])}, obj, doc)
      self.cl.delete()

      # 7
      obj = Decimal(-1.7e308, 1000, 2)
      doc = OrderedDict([("a", obj), ("_id", 1)])
      self.insert_decimal(self.cl, doc)
      self.check_result(self.cl, {"a": OrderedDict([("$type", 1), ("$et", 100)])}, obj, doc)
      self.cl.delete()

      # 8
      obj = Decimal(4.9e-324, 1000, 324)
      doc = OrderedDict([("a", obj), ("_id", 1)])
      self.insert_decimal(self.cl, doc)
      self.check_result(self.cl, {"a": OrderedDict([("$type", 1), ("$et", 100)])}, obj, doc)
      self.cl.delete()

      # 9
      obj = Decimal(-4.9e-324, 1000, 324)
      doc = OrderedDict([("a", obj), ("_id", 1)])
      self.insert_decimal(self.cl, doc)
      self.check_result(self.cl, {"a": OrderedDict([("$type", 1), ("$et", 100)])}, obj, doc)
      self.cl.delete()

      # 10
      obj = Decimal(-.49, 5, 2)
      doc = OrderedDict([("a", obj), ("_id", 1)])
      self.insert_decimal(self.cl, doc)
      self.check_result(self.cl, {"a": OrderedDict([("$type", 1), ("$et", 100)])}, obj, doc)
      self.cl.delete()

      # 11
      obj = Decimal("92233720368547758089223372036854775808", 1000, 0)
      doc = OrderedDict([("a", obj), ("_id", 1)])
      self.insert_decimal(self.cl, doc)
      self.check_result(self.cl, {"a": OrderedDict([("$type", 1), ("$et", 100)])}, obj, doc)
      self.cl.delete()

      # 12
      obj = Decimal("-92233720368547758089223372036854775808", 1000, 2)
      doc = OrderedDict([("a", obj), ("_id", 1)])
      self.insert_decimal(self.cl, doc)
      self.check_result(self.cl, {"a": OrderedDict([("$type", 1), ("$et", 100)])}, obj, doc)
      self.cl.delete()

      # 13
      obj = Decimal("4.92513687945623587412589623", 27, 26)
      doc = OrderedDict([("a", obj), ("_id", 1)])
      self.insert_decimal(self.cl, doc)
      self.check_result(self.cl, {"a": OrderedDict([("$type", 1), ("$et", 100)])}, obj, doc)
      self.cl.delete()

      # 14
      obj = Decimal("-4.92513687945623587412589623", 27, 26)
      doc = OrderedDict([("a", obj), ("_id", 1)])
      self.insert_decimal(self.cl, doc)
      self.check_result(self.cl, {"a": OrderedDict([("$type", 1), ("$et", 100)])}, obj, doc)
      self.cl.delete()

      # 15
      obj = Decimal("0", 27, 26)
      doc = OrderedDict([("a", obj), ("_id", 1)])
      self.insert_decimal(self.cl, doc)
      self.check_result(self.cl, {"a": OrderedDict([("$type", 1), ("$et", 100)])}, obj, doc)
      self.cl.delete()

      # 16
      self.check_InvalidValueArg_Result("a", 10, 2)

      # seqDB-9463,17
      obj = Decimal("1", 1, 0)
      doc = OrderedDict([("a", obj), ("_id", 1)])
      self.insert_decimal(self.cl, doc)
      self.check_result(self.cl, {"a": OrderedDict([("$type", 1), ("$et", 100)])}, obj, doc)
      self.cl.delete()

      # 18
      obj = Decimal("1", 1000, 0)
      doc = OrderedDict([("a", obj), ("_id", 1)])
      self.insert_decimal(self.cl, doc)
      self.check_result(self.cl, {"a": OrderedDict([("$type", 1), ("$et", 100)])}, obj, doc)
      self.cl.delete()

      # 19
      self.check_InvalidValueArg_Result(1, 0, 0)

      # 20
      self.check_InvalidValueArg_Result(1, 1001, 0)

      # 21
      self.check_InvalidValueArg_Result(1, -1, 0)

      # 22
      self.check_InvalidTypeArg_Result(1, "a", 0)

      # 22
      self.check_InvalidTypeArg_Result(1, 10.2, 0)

      # seqDB-9464,23
      obj = Decimal("1", 1000, 0)
      doc = OrderedDict([("a", obj), ("_id", 1)])
      self.insert_decimal(self.cl, doc)
      self.check_result(self.cl, {"a": OrderedDict([("$type", 1), ("$et", 100)])}, obj, doc)
      self.cl.delete()

      # 24
      obj = Decimal("1", 1000, 999)
      doc = OrderedDict([("a", obj), ("_id", 1)])
      self.insert_decimal(self.cl, doc)
      self.check_result(self.cl, {"a": OrderedDict([("$type", 1), ("$et", 100)])}, obj, doc)
      self.cl.delete()

      # 25
      self.check_InvalidScaleArg_Result(1, 1000, 1000)

      # 26
      self.check_InvalidValueArg_Result(1, 1000, -1)

      # 27
      self.check_InvalidScaleTypeArg_Result(1, 1000, "a")

      # 28
      self.check_InvalidTypeArg_Result(1, 10.2, 0)

      # seqDB-94645,29
      obj = Decimal("123.56", 5, 2)
      doc = OrderedDict([("a", obj), ("_id", 1)])
      self.insert_decimal(self.cl, doc)
      self.check_result(self.cl, {"a": OrderedDict([("$type", 1), ("$et", 100)])}, obj, doc)
      self.cl.delete()

      # 30
      obj = Decimal("123.56", 5, 1)
      doc = OrderedDict([("a", obj), ("_id", 1)])
      self.insert_decimal(self.cl, doc)
      self.check_result(self.cl, {"a": OrderedDict([("$type", 1), ("$et", 100)])}, obj, doc)
      self.cl.delete()

      # 31
      obj = Decimal("123.56", 6, 3)
      doc = OrderedDict([("a", obj), ("_id", 1)])
      self.insert_decimal(self.cl, doc)
      self.check_result(self.cl, {"a": OrderedDict([("$type", 1), ("$et", 100)])}, obj, doc)
      self.cl.delete()

      # 32
      self.check_InvalidScaleArg_Result(123.1234, 2, 1)

      # 33
      self.check_InvalidScaleArg_Result(123, 7, 5)

      # seqDB-9466,34
      obj = Decimal(2147483647, None, None)
      doc = OrderedDict([("a", obj), ("_id", 1)])
      self.insert_decimal(self.cl, doc)
      self.check_result(self.cl, {"a": OrderedDict([("$type", 1), ("$et", 100)])}, obj, doc)
      self.cl.delete()

      # 35
      obj = Decimal(-2147483648, None, None)
      doc = OrderedDict([("a", obj), ("_id", 1)])
      self.insert_decimal(self.cl, doc)
      self.check_result(self.cl, {"a": OrderedDict([("$type", 1), ("$et", 100)])}, obj, doc)
      self.cl.delete()

      # 36
      obj = Decimal(0, None, None)
      doc = OrderedDict([("a", obj), ("_id", 1)])
      self.insert_decimal(self.cl, doc)
      self.check_result(self.cl, {"a": OrderedDict([("$type", 1), ("$et", 100)])}, obj, doc)
      self.cl.delete()

      # 37
      obj = Decimal(9223372036854775807, None, None)
      doc = OrderedDict([("a", obj), ("_id", 1)])
      self.insert_decimal(self.cl, doc)
      self.check_result(self.cl, {"a": OrderedDict([("$type", 1), ("$et", 100)])}, obj, doc)
      self.cl.delete()

      # 38
      obj = Decimal(-9223372036854775808, None, None)
      doc = OrderedDict([("a", obj), ("_id", 1)])
      self.insert_decimal(self.cl, doc)
      self.check_result(self.cl, {"a": OrderedDict([("$type", 1), ("$et", 100)])}, obj, doc)
      self.cl.delete()

      # 39
      obj = Decimal(1.7e308, None, None)
      doc = OrderedDict([("a", obj), ("_id", 1)])
      self.insert_decimal(self.cl, doc)
      self.check_result(self.cl, {"a": OrderedDict([("$type", 1), ("$et", 100)])}, obj, doc)
      self.cl.delete()

      # 40
      obj = Decimal(-1.7e308, None, None)
      doc = OrderedDict([("a", obj), ("_id", 1)])
      self.insert_decimal(self.cl, doc)
      self.check_result(self.cl, {"a": OrderedDict([("$type", 1), ("$et", 100)])}, obj, doc)
      self.cl.delete()

      # 41
      obj = Decimal(4.9e-324, None, None)
      doc = OrderedDict([("a", obj), ("_id", 1)])
      self.insert_decimal(self.cl, doc)
      self.check_result(self.cl, {"a": OrderedDict([("$type", 1), ("$et", 100)])}, obj, doc)
      self.cl.delete()

      # 42
      obj = Decimal(-4.9e-324, None, None)
      doc = OrderedDict([("a", obj), ("_id", 1)])
      self.insert_decimal(self.cl, doc)
      self.check_result(self.cl, {"a": OrderedDict([("$type", 1), ("$et", 100)])}, obj, doc)
      self.cl.delete()

      # 43
      obj = Decimal(-.49, None, None)
      doc = OrderedDict([("a", obj), ("_id", 1)])
      self.insert_decimal(self.cl, doc)
      self.check_result(self.cl, {"a": OrderedDict([("$type", 1), ("$et", 100)])}, obj, doc)
      self.cl.delete()

      # 44
      obj = Decimal("92233720368547758089223372036854775808", None, None)
      doc = OrderedDict([("a", obj), ("_id", 1)])
      self.insert_decimal(self.cl, doc)
      self.check_result(self.cl, {"a": OrderedDict([("$type", 1), ("$et", 100)])}, obj, doc)
      self.cl.delete()

      # 45
      obj = Decimal("-92233720368547758089223372036854775808", None, None)
      doc = OrderedDict([("a", obj), ("_id", 1)])
      self.insert_decimal(self.cl, doc)
      self.check_result(self.cl, {"a": OrderedDict([("$type", 1), ("$et", 100)])}, obj, doc)
      self.cl.delete()

      # 46
      obj = Decimal("4.92513687945623587412589623", None, None)
      doc = OrderedDict([("a", obj), ("_id", 1)])
      self.insert_decimal(self.cl, doc)
      self.check_result(self.cl, {"a": OrderedDict([("$type", 1), ("$et", 100)])}, obj, doc)
      self.cl.delete()

      # 47
      obj = Decimal("-4.92513687945623587412589623", None, None)
      doc = OrderedDict([("a", obj), ("_id", 1)])
      self.insert_decimal(self.cl, doc)
      self.check_result(self.cl, {"a": OrderedDict([("$type", 1), ("$et", 100)])}, obj, doc)
      self.cl.delete()

      # 48
      obj = Decimal("0", None, None)
      doc = OrderedDict([("a", obj), ("_id", 1)])
      self.insert_decimal(self.cl, doc)
      self.check_result(self.cl, {"a": OrderedDict([("$type", 1), ("$et", 100)])}, obj, doc)
      self.cl.delete()

      # 49
      self.check_InvalidValueArg_Result("a", None, None)

      # seqDB-9468,50
      obj = Decimal("123", None, None)
      obj.set_zero()
      doc = OrderedDict([("a", obj), ("_id", 1)])
      self.insert_decimal(self.cl, doc)
      self.check_result(self.cl, {"a": OrderedDict([("$type", 1), ("$et", 100)])}, obj, doc)

      self.assertEqual(obj.compare(0), 0)

      rc = obj.is_zero()
      self.assertTrue(rc)

      self.cl.delete()

      # seqDB-9469,51
      obj = Decimal("123", None, None)
      obj.set_max()
      doc = OrderedDict([("a", obj), ("_id", 1)])
      self.insert_decimal(self.cl, doc)
      self.check_result(self.cl, {"a": OrderedDict([("$type", 1), ("$et", 100)])}, obj, doc)

      obj1 = Decimal("9e+1000", None, None)
      self.assertEqual(obj.compare(obj1), 1)

      rc = obj.is_max()
      self.assertTrue(rc)
      self.cl.delete()

      # seqDB-9470,52
      obj = Decimal("123", None, None)
      obj.set_min()
      doc = OrderedDict([("a", obj), ("_id", 1)])
      self.insert_decimal(self.cl, doc)
      self.check_result(self.cl, {"a": OrderedDict([("$type", 1), ("$et", 100)])}, obj, doc)

      obj1 = Decimal("-9e-1000", None, None)
      self.assertEqual(obj.compare(obj1), -1)

      rc = obj.is_min()
      self.assertTrue(rc)
      self.cl.delete()

      # seqDB-9471,53,to_int():python2 to long,python3 to int
      obj = Decimal("123", None, None)
      rc = obj.to_int()
      self.assertEqual(obj.compare(rc), 0)

      # 54
      obj = Decimal("123.12", None, None)
      rc = obj.to_int()
      self.assertEqual(rc, 123)

      # 55
      obj = Decimal("-9223372036854775808", None, None)
      rc = obj.to_int()
      self.assertEqual(rc, -9223372036854775808)

      # 56
      obj = Decimal("9223372036854775807", None, None)
      rc = obj.to_int()
      self.assertEqual(rc, 9223372036854775807)

      # 57
      obj = Decimal("-9223372036854775809", None, None)
      rc = obj.to_int()
      self.assertEqual(rc, 0)

      # 58
      obj = Decimal("9223372036854775808", None, None)
      rc = obj.to_int()
      self.assertEqual(rc, 0)

      # seqDB-9472,59
      obj = Decimal("123.12", None, None)
      rc = obj.to_float()
      self.assertEqual(type(rc), float)
      self.assertEqual(obj.compare(rc), 0)

      # 60
      obj = Decimal("123", None, None)
      rc = obj.to_float()
      self.assertEqual(type(rc), float)
      self.assertEqual(obj.compare(rc), 0)

      # 61
      obj = Decimal("1.7e+308", None, None)
      rc = obj.to_float()
      self.assertEqual(type(rc), float)
      self.assertEqual(obj.compare(rc), 0)

      # 62
      obj = Decimal("-1.7e+308", None, None)
      rc = obj.to_float()
      self.assertEqual(type(rc), float)
      self.assertEqual(obj.compare(rc), 0)

      # 63
      obj = Decimal("4.9e-324", None, None)
      rc = obj.to_float()
      self.assertEqual(type(rc), float)
      self.assertEqual(rc, 4.94065645841e-324)

      # 64
      obj = Decimal("-4.9e-324", None, None)
      rc = obj.to_float()
      self.assertEqual(type(rc), float)
      self.assertEqual(rc, -4.94065645841e-324)

      # 65
      obj = Decimal("1.7e+309", None, None)
      rc = obj.to_float()
      self.assertEqual(type(rc), float)
      self.assertEqual(str(rc), "inf")

      # 66
      obj = Decimal("-1.7e+309", None, None)
      rc = obj.to_float()
      self.assertEqual(type(rc), float)
      self.assertEqual(str(rc), "-inf")

      # seqDB-9473,67
      obj = Decimal("123.12", None, None)
      rc = obj.to_string()
      self.assertEqual(type(rc), str)
      self.assertEqual(rc, "123.12")

      # 68
      obj = Decimal("123", None, None)
      rc = obj.to_string()
      self.assertEqual(type(rc), str)
      self.assertEqual(rc, "123")

      # seqDB-9474,69
      obj = Decimal("123.12", None, None)
      self.assertEqual(obj.compare(123.12), 0)
      self.assertEqual(obj.compare(123), 1)

      obj1 = Decimal("123.12", 5, 2)
      self.assertEqual(obj.compare(obj1), 0)

      # 70
      obj = Decimal("123.12", 5, 2)
      self.assertEqual(obj.compare(123.12), 0)
      self.assertEqual(obj.compare(123), 1)

      obj1 = Decimal("123.12", None, None)
      self.assertEqual(obj.compare(obj1), 0)

      # seqDB-9467,71
      i = 0
      value = ""
      while (i < 131072):
         value = value + "9"
         i = i + 1
      j = 0
      value = value + "."
      while (j < 16383):
         value = value + "8"
         j = j + 1
      obj = Decimal(value, None, None)
      doc = OrderedDict([("a", obj), ("_id", 1)])
      self.insert_decimal(self.cl, doc)
      self.check_result(self.cl, {"a": OrderedDict([("$type", 1), ("$et", 100)])}, obj, doc)
      self.cl.delete()

      # 72
      i = 0
      value = ""
      while (i <= 131072):
         value = value + "9"
         i = i + 1
      j = 0
      value = value + "."
      while (j <= 16383):
         value = value + "8"
         j = j + 1
      self.check_InvalidValueArg_Result(value, None, None)

      # 73
      i = 0
      value = ""
      while (i <= 131072):
         value = value + "9"
         i = i + 1
      self.check_InvalidValueArg_Result(value, None, None)

      # 74
      j = 0
      value = "1"
      value = value + "."
      while (j <= 16383):
         value = value + "8"
         j = j + 1
      self.check_InvalidValueArg_Result(value, None, None)

      # seqDB-10493
      obj = Decimal(2147483647, 100, 2)
      json = '{"$decimal": "2147483647.00", "$precision": [100, 2]}'
      self.assertEqual(dumps(obj), json)
      self.assertEqual(loads(json).compare(obj), 0)
      self.assertEqual(dumps(obj), json)
      self.assertEqual(loads(json).compare(obj), 0)
      self.assertEqual(loads(dumps(obj)).compare(obj), 0)
      self.assertEqual(dumps(loads(json)), json)

      obj = Decimal(-9223372036854775808, 100, 2)
      json = '{"$decimal": "-9223372036854775808.00", "$precision": [100, 2]}'
      self.assertEqual(dumps(obj), json)
      self.assertEqual(loads(json).compare(obj), 0)
      self.assertEqual(dumps(obj), json)
      self.assertEqual(loads(json).compare(obj), 0)
      self.assertEqual(loads(dumps(obj)).compare(obj), 0)
      self.assertEqual(dumps(loads(json)), json)

      obj = Decimal(4.9e-324, 1000, 325)
      json = '{"$decimal": "0.0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000049", "$precision": [1000, 325]}'
      self.assertEqual(dumps(obj), json)
      self.assertEqual(loads(json).compare(obj), 0)
      self.assertEqual(dumps(obj), json)
      self.assertEqual(loads(json).compare(obj), 0)
      self.assertEqual(loads(dumps(obj)).compare(obj), 0)
      self.assertEqual(dumps(loads(json)), json)

      obj = Decimal(2147483647, None, None)
      json = '{"$decimal": "2147483647"}'
      self.assertEqual(dumps(obj), json)
      self.assertEqual(loads(json).compare(obj), 0)
      self.assertEqual(dumps(obj), json)
      self.assertEqual(loads(json).compare(obj), 0)
      self.assertEqual(loads(dumps(obj)).compare(obj), 0)
      self.assertEqual(dumps(loads(json)), json)

      obj = Decimal(-9223372036854775808, None, None)
      json = '{"$decimal": "-9223372036854775808"}'
      self.assertEqual(dumps(obj), json)
      self.assertEqual(loads(json).compare(obj), 0)
      self.assertEqual(dumps(obj), json)
      self.assertEqual(loads(json).compare(obj), 0)
      self.assertEqual(loads(dumps(obj)).compare(obj), 0)
      self.assertEqual(dumps(loads(json)), json)

      obj = Decimal("4.9e-324", None, None)
      json = '{"$decimal": "0.0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000049"}'
      self.assertEqual(dumps(obj), json)
      self.assertEqual(loads(json).compare(obj), 0)
      self.assertEqual(dumps(obj), json)
      self.assertEqual(loads(json).compare(obj), 0)
      self.assertEqual(loads(dumps(obj)).compare(obj), 0)
      self.assertEqual(dumps(loads(json)), json)

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)

   def insert_decimal(self, cl, data):
      try:
         cl.insert(data)
      except SDBBaseError as e:
         self.fail("insert_fail,data:" + str(data))

   def check_result(self, cl, condition, obj, doc):
      cnt = cl.get_count(condition=condition)
      rec = cl.query(condition=condition).next()

      self.assertEqual(cnt, 1)
      self.assertEqual(rec['a'].compare(obj), 0)
      self.assertEqual(dumps(rec.pop('_id')), dumps(doc.pop('_id')))

   def check_InvalidValueArg_Result(self, data, precision, scale):
      try:
         obj = Decimal(data, precision, scale)
         self.fail("need_an_error")
      except Exception as err:
         print(err)

   def check_InvalidTypeArg_Result(self, data, precision, scale):
      try:
         obj = Decimal(data, precision, scale)
         self.fail("need_an_error")
      except Exception as err:
         print(err)

   def check_InvalidScaleArg_Result(self, data, precision, scale):
      try:
         obj = Decimal(data, precision, scale)
         self.fail("need_an_error")
      except Exception as err:
         print(err)

   def check_InvalidScaleTypeArg_Result(self, data, precision, scale):
      try:
         obj = Decimal(data, precision, scale)
         self.fail("need_an_error")
      except Exception as err:
         print(err)

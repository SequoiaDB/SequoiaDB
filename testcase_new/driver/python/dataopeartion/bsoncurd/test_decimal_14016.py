
from bson.decimal import Decimal
from bson.objectid import ObjectId
from lib import  testlib

class TestDecimal14016(testlib.SdbTestBase):
   def setUp(self):
      # create cs and cl
      self.cs_name = "cs_14016"
      self.cl_name = "cl_14016"
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name, {"ReplSize": 0})

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)

   def test(self):
      # test special value
      special_values = [Decimal("MAX"), Decimal("MIN"), Decimal("NAN"), Decimal("-1"), Decimal("+1"), Decimal(".1")]
      # for decimal_value in special_values:
      for v in special_values:
         expect = {"_id": ObjectId(), "decimal": v}
         self.cl.insert(expect)
         cur = self.cl.query(condition={"_id": expect["_id"]})
         actual = cur.next()
         cur.close()
         self.assertEqual(actual, expect)
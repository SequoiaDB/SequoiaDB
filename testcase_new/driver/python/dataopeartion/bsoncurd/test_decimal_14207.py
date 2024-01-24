from bson.decimal import Decimal
from bson.objectid import ObjectId
import random
from lib import  testlib

class TestDecimal14207(testlib.SdbTestBase):
   def setUp(self):
      # create cs and cl
      self.cs_name = "cs_14207"
      self.cl_name = "cl_14207"
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name, {"ReplSize": 0})

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)

   def test(self):
      v=random.randint(-100,100)/random.randint(1,100)
      d1=Decimal(v)
      d2=Decimal(v)
      self.assertTrue(d1==d2)
      self.assertFalse(d1!=d2)

      d1=Decimal(v)
      d2=Decimal(v+1)
      self.assertTrue(d1!=d2)
      self.assertFalse(d1==d2)





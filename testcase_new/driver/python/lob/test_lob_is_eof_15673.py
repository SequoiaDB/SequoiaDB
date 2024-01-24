# @decription read lob ues is_eof() interface
# @testlink   seqDB-15673
# @interface  is_eof	( self )		
# @author     yinzhen 2018-10-22

from lib import testlib
from bson.objectid import ObjectId
import string
import random

class TestLobIsEof(testlib.SdbTestBase):
   def setUp(self):
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)
		 
   def test_is_eof_15673(self):
      self.lob_size = 1024
      self.lob_context = self.random_str(self.lob_size)
      self.md5 = self.get_md5(self.lob_context)

      # put lob   
      self.oid_set = set()	 
      lob = self.cl.create_lob()
      lob.write(self.lob_context, len(self.lob_context))
      oid = lob.get_oid()
      self.oid_set.add(oid)
      lob.close()
	  
      lob = self.cl.get_lob(oid)
	  
      # do not finish read lob
      randomNum = random.randint(1,1000)
      context = lob.read(randomNum)
      print ("First read length ", randomNum, " size lob, isEof : ", lob.is_eof())
      if lob.is_eof():
         self.fail("implement isEof() failed when already not finish read")	  
	  
      # finish read lob
      context = context + lob.read(1024)
      print ("Last read length ", 1024 - randomNum, " size lob, isEof : ", lob.is_eof())
      if not lob.is_eof():
         self.fail("implement isEof() failed when already finish read")	  
	  
      # check lob md5
      self.check_lob_md5(lob, context)  	  
	
   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)
		 
   def check_lob_md5(self, lob, context):
      if self.get_md5(context) != self.md5:
         self.fail("lobid: " + lob.get_oid() + " except md5: " + self.md5)
      if lob.get_size() != self.lob_size:
         self.fail(
            "lobid: " + str(lob.get_oid()) + " except size: " + str(self.lob_size) + "actually: " + str(lob.get_size()))
	  
   def random_str(self, length):
      s = ""
      r = random.Random()
      for i in range(length):
         s += r.choice(string.ascii_letters + string.digits)
      return s
   
   def get_md5(self, text):
      import hashlib
      if isinstance(text, str):
         m2 = text.encode("utf-8")
      else:
         m2 = text
      return hashlib.sha256(m2).hexdigest()
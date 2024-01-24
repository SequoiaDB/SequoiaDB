# @decription: test ssl,open ssl=true
# @testlink:   seqDB-9560
# @author:     liuxiaoxuan 2017-9-09

import unittest
import datetime
from pysequoiadb.error import (SDBTypeError, SDBBaseError, SDBEndOfCursor, SDBError)
from pysequoiadb import client
from lib import sdbconfig
from lib import testlib

class TestSSL9560(testlib.SdbTestBase):
    def setUp(self):
        self.config = sdbconfig.SdbConfig()
        self.db = client(self.config.host_name, self.config.service, '', '', False)

    def test_ssl_9560(self):
       # ssl = false
       testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
       self.cs = self.db.create_collection_space(self.cs_name)
       self.cl = self.cs.create_collection(self.cl_name)
       # ssl = true
       self.check_with_ssl()

    def tearDown(self):
      if self.should_clean_env():
         try:	
            self.db.drop_collection_space(self.cs_name) 
         except SDBBaseError as e:
            self.assertEqual(-34, e.code, 'tearDown fail,errmsg: ' + e.detail)			

    def check_with_ssl(self):
       new_db = None
       try:
          new_db = client(self.config.host_name, self.config.service, '', '', True)
          new_db.drop_collection_space(self.cs_name)
       except SDBBaseError as e:
          self.fail('ssl fail: ' + e.detail)
       finally:
          if not new_db == None:
             new_db.disconnect()

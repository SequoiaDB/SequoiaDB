# @decription: test ssl,not open ssl=true
# @testlink:   seqDB-9561
# @author:     liuxiaoxuan 2017-9-09

import unittest
import datetime
from pysequoiadb.error import (SDBTypeError, SDBBaseError, SDBEndOfCursor, SDBError)
from pysequoiadb import client
from lib import sdbconfig
from lib import testlib

class TestSSL9561(testlib.SdbTestBase):
    def setUp(self):
        self.config = sdbconfig.SdbConfig()
        self.db = client(self.config.host_name, self.config.service, '', '', False)

    def test_ssl_9561(self):
       testlib.drop_cs(self.db, self.cs_name, ignore_not_exist = True)
       self.cs = self.db.create_collection_space(self.cs_name)
       self.cl = self.cs.create_collection(self.cl_name)
       self.check_with_ssl()

    def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)  
			
    def check_with_ssl(self):
       new_db = None
       try:
          new_db = client(self.config.host_name, self.config.service, '', '', True)
          self.fail('NEED SSL FAIL')
       except SDBBaseError as e:
          self.assertTrue(True)
       finally:
          if not new_db == None:
             new_db.disconnect()

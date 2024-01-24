# -*- coding: utf-8 -*-

# @testlink   seqDB-22425
# @interface  lob.get_run_time_detail()
# @author     Zixian Yan 2020-07-30

from lob import util
from lib import testlib
from pysequoiadb import lob
from pysequoiadb.lob import LOB_WRITE

class Lob_22425(testlib.SdbTestBase):
   def setUp(self):
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)

   def test_main(self):
       # Create a lob with 1024 bytes data;
       data_size = 1024
       lob_content = util.random_str(data_size)
       lob = self.cl.create_lob()
       lob.write(lob_content, len(lob_content))

       lob_detail = lob.get_run_time_detail()
       lob_access_info = lob_detail['AccessInfo']

       expect_detail = {'ReadCount': 0,
                       'WriteCount': 0,
                       'RefCount': 1,
                       'LockSections': [],
                       'ShareReadCount': 0}
       # Compare their detail information; If they doesn't match, raise Exception
       self.assertEqual( lob_access_info, expect_detail )

       lob.close()

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)

# @decription: cancel non exist task
# @testlink:   seqDB-13686
# @author:     liuxiaoxuan 2017-12-07

from lib import testlib
from bson.py3compat import (long_type)
from pysequoiadb.error import (SDBBaseError, SDBError)

class taskException13686(testlib.SdbTestBase):
   def setUp(self):
      if testlib.is_standalone():
         self.skipTest('skip standalone')

   def test_task_13686(self):
      # check cancel non exist task
      task_id_non_exsit = long_type(999999)
      is_sync = True
      self.check_error_cancel_task(task_id_non_exsit, is_sync)
      
   def check_error_cancel_task(self, task_id, is_sync):
      try:
         self.db.cancel_task(task_id, is_sync)     
         self.fail("NEED SDB ERROR")         
      except SDBError as e:
         self.assertEqual(e.code, -173)
         self.assertEqual(e.detail, "Failed to cancel task")   

   def tearDown(self):
      pass          

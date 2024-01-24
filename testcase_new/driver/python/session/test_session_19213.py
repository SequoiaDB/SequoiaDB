# @testlink:   seqDB-19213
# @author:     yinzhen 2019-09-03

import unittest
from lib import testlib
from lib import sdbconfig
from session.commlib import *
from pysequoiadb.error import SDBBaseError

class TestgetSession19213(testlib.SdbTestBase):
   def setUp(self):
      pass

   def test_session_19213(self):
      # check default session attr
      default_session_attr = {'TransTimeout': 60, 'TransRCCount': True, 'Source': '', 'TransAutoCommit': False, 'TransLockWait': False, 'TransAutoRollback': True, 'PreferedInstanceMode': 'random', 'PreferedInstance': 'M', 'TransUseRBS': True, 'Timeout': -1, 'TransIsolation': 0}
      current_session_attr = self.db.get_session_attri()
      act_session_attr = self.get_act_session_attr(default_session_attr, current_session_attr)
      self.assertEqual(default_session_attr, act_session_attr)

      # set session attr transisolation 2
      self.db.set_session_attri({'TransIsolation':2})
      exp_session_attr = {'TransTimeout': 60, 'TransRCCount': True, 'Source': '', 'TransAutoCommit': False, 'TransLockWait': False, 'TransAutoRollback': True, 'PreferedInstanceMode': 'random', 'PreferedInstance': 'M', 'TransUseRBS': True, 'Timeout': -1, 'TransIsolation': 2}
      current_session_attr = self.db.get_session_attri()
      act_session_attr = self.get_act_session_attr(exp_session_attr, current_session_attr)
      self.assertEqual(exp_session_attr, act_session_attr)
      
      # update config transisolation 0 transtimeout 120
      self.db.update_config(configs={'TransIsolation': 0, 'transactiontimeout': 120}, options={})
      current_session_attr = self.db.get_session_attri()
      act_session_attr = self.get_act_session_attr(exp_session_attr, current_session_attr)
      self.assertEqual(exp_session_attr, act_session_attr)
      
      # cleanup cache
      self.db.set_session_attri({})
      
      # get session
      exp_session_attr = {'TransTimeout': 120, 'TransRCCount': True, 'Source': '', 'TransAutoCommit': False, 'TransLockWait': False, 'TransAutoRollback': True, 'PreferedInstanceMode': 'random', 'PreferedInstance': 'M', 'TransUseRBS': True, 'Timeout': -1, 'TransIsolation': 2}
      current_session_attr = self.db.get_session_attri()
      act_session_attr = self.get_act_session_attr(exp_session_attr, current_session_attr)
      self.assertEqual(exp_session_attr, act_session_attr)
      
      # get session by param False
      current_session_attr = self.db.get_session_attri(False)
      act_session_attr = self.get_act_session_attr(exp_session_attr, current_session_attr)
      self.assertEqual(exp_session_attr, act_session_attr)
      
   def tearDown(self):
      self.db.update_config(configs={'TransIsolation': 0, 'transactiontimeout': 60}, options={})
      
   def get_act_session_attr(self, exp_fields, cur_session_attr):
      exp_session_attr = {}
      for exp_field in exp_fields:
         for cur_field in cur_session_attr:
            if exp_field == cur_field:
               exp_session_attr[exp_field] = cur_session_attr[cur_field]
               break
      return exp_session_attr
               
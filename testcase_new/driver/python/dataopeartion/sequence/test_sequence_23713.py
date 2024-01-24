# @decription: test sequence
# @testlink:   seqDB-23713
# @interface:  create_sequence/drop_sequence/get_sequence/rename_sequence
#              fetch/get_current_value/get_next_value/restart/set_attributes/set_current_value
# @author:     fanyu 2020-03-18

from lib import testlib
from pysequoiadb.error import SDBBaseError
from pysequoiadb.client import (SDB_SNAP_SEQUENCES)


class TestSequence23713(testlib.SdbTestBase):
    def setUp(self):
        if testlib.is_standalone():
            self.skipTest("skip! This testcase do not support standlone")
        self.seq_name = "py_seq_23713"
        self.seq_name_new = "py_seq_23713_new"
        # clear env
        try:
            self.db.drop_sequence(self.seq_name)
        except SDBBaseError as e:
            if -324 != e.code:
                self.fail("drop_seq_fail,detail:" + str(e))
        try:
            self.db.drop_sequence(self.seq_name_new)
        except SDBBaseError as e:
            if -324 != e.code:
                self.fail("drop_seq_fail,detail:" + str(e))

    def test(self):
        # create sequence
        options = {'StartValue': 10, 'MinValue': 10, 'MaxValue': 23701, 'Increment': 10, 'CacheSize': 1, 'AcquireSize': 1,'Cycled': True }
        self.db.create_sequence(self.seq_name, options)

        # snapshot sequence
        condition = {'Name': self.seq_name}
        selector = {'AcquireSize': 1, 'CacheSize': 1, 'CurrentValue': 1, 'Cycled': 1, 'Increment': 1, 'MaxValue': 1, 'MinValue': 1, 'StartValue': 1}
        cursor = self.db.get_snapshot(snap_type=SDB_SNAP_SEQUENCES, condition=condition, selector=selector)
        act_results = testlib.get_all_records(cursor)
        act_result = act_results[0]
        exp_result = {'StartValue': 10, 'MinValue': 10, 'MaxValue': 23701, 'Increment': 10,'CacheSize': 1, 'AcquireSize': 1, 'Cycled': True, 'CurrentValue': 10}
        # check snapshot
        self.assertDictEqual(act_result, exp_result)

        # get sequence
        seq = self.db.get_sequence(self.seq_name)

        # get next value
        next_value = seq.get_next_value()
        self.assertEqual(next_value, exp_result['MinValue'])

        # get current value
        current_value = seq.get_current_value()
        self.assertEqual(current_value, exp_result['MinValue'])

        # set current value
        seq.set_current_value((exp_result['MinValue'] + 20))
        current_value = seq.get_current_value()
        self.assertEqual(current_value, (exp_result['MinValue'] + 20))

        # rename sequence
        self.db.rename_sequence(self.seq_name, self.seq_name_new)
        seq = self.db.get_sequence(self.seq_name_new)

        # restart
        start_value = exp_result['MinValue'] + 10
        seq.restart(start_value)
        next_value = seq.get_next_value()
        self.assertEqual(next_value, start_value)

        # fetch
        fetch_result = seq.fetch(1)
        exp_fetch_result = {'return_num': 1, 'next_value': 30, 'increment': 10}
        self.assertDictEqual(fetch_result, exp_fetch_result)

        # set attributes
        attributes = {'MinValue': 1, 'MaxValue': 10};
        seq.set_attributes(attributes)
        next_value = seq.get_next_value()
        self.assertEqual(next_value, attributes['MinValue'])

        # drop sequence
        self.db.drop_sequence(self.seq_name_new)
        # check
        try:
            self.db.drop_sequence(self.seq_name_new)
            self.fail("exp failed but act success!!!!")
        except SDBBaseError as e:
            if -324 != e.code:
                self.fail("drop_seq_fail,detail:" + str(e))

    def tearDown(self):
        if self.should_clean_env():
            try:
                self.db.drop_sequence(self.seq_name)
            except SDBBaseError as e:
                if -324 != e.code:
                    self.fail("drop_seq_fail,detail:" + str(e))
            try:
                self.db.drop_sequence(self.seq_name_new)
            except SDBBaseError as e:
                if -324 != e.code:
                    self.fail("drop_seq_fail,detail:" + str(e))
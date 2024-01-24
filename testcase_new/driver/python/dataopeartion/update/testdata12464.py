# @decription: data  opeartion
# @testlink:   seqDB-12464
# @author:     LaoJingTang 2017-8-30

from lib import testlib
from pysequoiadb.error import SDBBaseError
from pysequoiadb.collection import (INSERT_FLG_RETURN_OID, UPDATE_FLG_RETURNNUM, QUERY_FLG_FORCE_HINT)


class Data12464Sdb(testlib.SdbTestBase):
    def setUp(self):
        testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
        self.cs = self.db.create_collection_space(self.cs_name)
        self.cl = self.cs.create_collection(self.cl_name)

    def update_test(self, record_to_insert, expect_list, update_rule, **kwargs):
        self.cl.bulk_insert(0, record_to_insert)
        self.cl.update(update_rule, **kwargs)
        list1 = testlib.get_all_records_noid(self.cl.query())
        self.assertListEqualUnordered(expect_list, list1)
        self.cl.delete()

    def test12464(self):
        record_to_insert = ({"a": 0, "b": 0}, {"a": 1, "b": 1}, {"a": 2, "b": 2})
        update_rule = {"$inc": {"a": 1}}

        # condition+update
        condition = {"a": {"$et": 0}}
        expect_list = ({"a": 1, "b": 0}, {"a": 1, "b": 1}, {"a": 2, "b": 2})
        self.update_test(record_to_insert, expect_list, update_rule, condition=condition)

        # hint+update
        hint = {"": "index"}
        expect = ({"a": 1, "b": 0}, {"a": 2, "b": 1}, {"a": 3, "b": 2})
        self.update_test(record_to_insert, expect, update_rule, hint=hint)

        # flags+update
        try:
            self.cl.update(update_rule,flags=QUERY_FLG_FORCE_HINT, hint=hint)
            self.assertFalse("should error but succeed")
        except SDBBaseError as e:
            if e.code != -53:
                raise Exception(e)

        # test update with DELETE_FLG_RETURNNUM
        self.cl.bulk_insert(INSERT_FLG_RETURN_OID, record_to_insert)
        update_rule = {"$set": {"a": 10}}
        ret_value = self.cl.update(update_rule, condition={}, flags=UPDATE_FLG_RETURNNUM)
        self.assertEqual({"ModifiedNum": 3, "UpdatedNum": 3, "InsertedNum": 0}, ret_value)
        self.assertEqual(self.cl.get_count({"a": 10}), len(record_to_insert))

    def tearDown(self):
        if self.should_clean_env():
            self.db.drop_collection_space(self.cs_name)

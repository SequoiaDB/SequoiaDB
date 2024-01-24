# @decription create_lob_id and list_lobs interface
# @testlink   seqDB-22225 seqDB-22226
# @interface  create_lob_id()  list_lobs()
# @author     fanyu 2020-05-28

from bson.objectid import ObjectId
from bson.py3compat import (long_type)
from lib import testlib
from lob import util
import time
from pysequoiadb.error import (SDBBaseError, SDBEndOfCursor)


class Lob22225And22226(testlib.SdbTestBase):
    def setUp(self):
        testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
        self.cs = self.db.create_collection_space(self.cs_name)
        self.cl = self.cs.create_collection(self.cl_name)

    def test_create_lob_id_and_list_lobs(self):
        self.lob_num = 10
        self.create_lob_id_test(num=self.lob_num)
        self.create_lob()
        self.check_lob_content()
        self.list_lobs_test()

    def create_lob_id_test(self, num=10):
        self.oid_list = []
        for i in range(2, num):
            if i % 2 == 0:
                # specify timestamp
                timestamp = time.strftime("%Y-%m-%d-%H.%M.%S", time.localtime())
                oid = self.cl.create_lob_id(timestamp)
            else:
                # not specify timestamp
                oid = self.cl.create_lob_id()
            self.oid_list.append(oid)

        # specify special timestamp
        oid = self.cl.create_lob_id("1902-01-01-00.00.00")
        self.oid_list.append(oid)
        oid = self.cl.create_lob_id("2037-12-31-23.59.59")
        self.oid_list.append(oid)

        #  specify invalid timestamp
        timestamp = time.strftime("%Y-%m-%d-%H-%M-%S", time.localtime())
        try:
            self.cl.create_lob_id(timestamp)
            self.fail("exp fail but act success!!!")
        except SDBBaseError as e:
            self.assertEqual(e.code, -6, e)

    def create_lob(self):
        i = 0
        self.base_lob_size = 1024
        self.lob_md5_list = []
        for oid in self.oid_list:
            lob = self.cl.create_lob(oid)
            lob_content = util.random_str(self.base_lob_size + i)
            self.lob_md5_list.append(util.get_md5(lob_content))
            lob.write(lob_content, self.base_lob_size + i)
            lob.close
            i = i + 1

    def check_lob_content(self):
        for i in range(len(self.oid_list)):
            lob = self.cl.get_lob(self.oid_list[i])
            content = lob.read(lob.get_size())
            self.assertEqual(util.get_md5(content), self.lob_md5_list[i], "lodId=" + str(self.oid_list[i]))

    def list_lobs_test(self):
        # list lobs with condition/selected/hint/order_by/num_to_skip/num_to_Return when cl has records
        cursor = self.cl.list_lobs(
            condition={"Size": {"$gte": self.base_lob_size, "$lt": (self.base_lob_size + self.lob_num)}},
            selected={"Size": {"$include": 1}, "Oid": {"$include": 1}}, hint={"": "Oid"},
            order_by={"Size": 1}, num_to_skip=1, num_to_Return=self.lob_num)
        # check result
        i = 0
        while True:
            try:
                rc = cursor.next()
                i = i + 1
                self.assertDictEqual(rc, {"Oid": self.oid_list[i], "Size": long_type((self.base_lob_size + i))})
            except SDBEndOfCursor:
                break
        cursor.close()
        self.assertEqual(i, self.lob_num - 1)

        # list pieces
        cursor = self.cl.list_lobs(
            condition={},
            selected={}, hint={"ListPieces": 1},
            order_by={"Size": 1}, num_to_skip=0, num_to_Return=self.lob_num)
        # check result
        i = 0
        while True:
            try:
                rc = cursor.next()
                self.assertDictEqual(rc, {"Oid": self.oid_list[i], "Sequence": 0})
                i = i + 1
            except SDBEndOfCursor:
                break
        cursor.close()
        self.assertEqual(i, self.lob_num)

    def tearDown(self):
        if self.should_clean_env():
            self.db.drop_collection_space(self.cs_name)

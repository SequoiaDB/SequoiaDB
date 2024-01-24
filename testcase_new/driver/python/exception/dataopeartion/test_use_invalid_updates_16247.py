# @decription updating records with invalid updates
# @testlink   seqDB-16247
# @author     yinzhen 2018-10-24

from lib import testlib
from lib import sdbconfig
from pysequoiadb.error import (SDBBaseError, SDBEndOfCursor)


class TestUseInvalidUpdates16247(testlib.SdbTestBase):
    def setUp(self):
        testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
        self.cs = self.db.create_collection_space(self.cs_name)

    def test_use_invalid_updates_16247(self):
        global actResult
        self.data_rg_name = "data16247"

        # create data rg
        data_rg = self.db.create_replica_group(self.data_rg_name)

        # create and start node
        data_hostname = self.db.get_replica_group_by_name("SYSCatalogGroup").get_master().get_hostname()
        service_name = str(sdbconfig.sdb_config.rsrv_port_begin)
        data_dbpath = sdbconfig.sdb_config.rsrv_node_dir + service_name
        data_rg.create_node(data_hostname, service_name, data_dbpath)
        data_rg.start()

        # create collection and insert records
        self.cl = self.cs.create_collection(self.cl_name, options={"Group": self.data_rg_name})
        self.cl.insert({"name": "zsan"})

        # updating records with invalid updates
        updater = "$est2"
        try:
            self.cl.update({updater: {"name": "lisi"}})
            self.fail("should error but success")
        except SDBBaseError as e:
            self.assertEqual(e.code, -6)
            actResult = e.error_object

        # check result
        expResult = {"errno": -6, "description": "Invalid Argument", "UpdatedNum": 0, "ModifiedNum": 0,
                     "InsertedNum": 0, "detail": "Failed to update, Updator operator[" + updater + "] error",
                     "ErrNodes": [
                         {"NodeName": data_hostname + ":" + service_name, "GroupName": self.data_rg_name, "Flag": -6,
                          "ErrInfo": {"errno": -6, "description": "Invalid Argument", "UpdatedNum": 0, "ModifiedNum": 0,
                                      "InsertedNum": 0, "detail": "Updator operator[" + updater + "] error"}}]}
        msg = str(expResult) + "expect is not equal to actResult " + str(actResult)
        self.assertDictEqual(expResult, actResult, msg)

    def tearDown(self):
        if self.should_clean_env():
            self.db.drop_collection_space(self.cs_name)
            self.db.remove_replica_group(self.data_rg_name)
            self.db.disconnect()

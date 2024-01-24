/******************************************************************************
*@Description : test sdblobtool import/export/migration with Legal User
*@Modify list :
*               2016-06-22   XueWang Liang  Init
*               覆盖测试用例8802/8803
******************************************************************************/
import("../lib/lobtool_migration.js");
var testPara = { user: "sdbadmin", passwd: "sdbadmin" };
// Test
lobToolMain(main, testPara);

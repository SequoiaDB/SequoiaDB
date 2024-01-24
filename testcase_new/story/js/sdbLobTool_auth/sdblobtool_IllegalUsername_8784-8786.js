/******************************************************************************
*@Description : test sdblobtool import/export/migration with Illegal Username
*@Modify list :
*               2016-06-22   XueWang Liang  Init
*               覆盖测试用例8784/8785/8786
******************************************************************************/
import("../lib/lobtool_migration.js");

var testPara = { user: "sdbadmin", passwd: "sdbadmin" };
LobToolTest.prototype.test = function () {
   // sdblobtool 迁移的选项参数
   var args = buildMigrationArgs(this.conf.srcFullCL, this.conf.dstFullCL, testPara.user, testPara.passwd,
      testPara.user, "sequoiadb", this.conf.useSsl);

   var errCode = -179;
   // 创建导入集合并将大对象导入
   this.otherCl = commCreateCL(db, COMMCSNAME, this.otherCLName);

   handleToolError(args, errCode);

   args["dstusrname"] = "root";
   args["dstpasswd"] = testPara.passwd;
   handleToolError(args, errCode);
}

LobToolTest.prototype.checkResult = function () {
}
// Test
lobToolMain(main, testPara);

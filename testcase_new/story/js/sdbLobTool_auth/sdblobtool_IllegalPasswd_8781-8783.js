/******************************************************************************
*@Description : test sdblobtool import/export/migration with Illegal Passwd
*@Modify list :
*               2016-06-22   XueWang Liang  Init
*               覆盖测试用例8781/8782/8783
******************************************************************************/
import("../lib/lobtool_export_import.js");

var testPara = { user: "sdbadmin", passwd: "sdbadmin" };
LobToolTest.prototype.test = function () {
   var args = buildImportOrExprtArgs("export", this.conf.expFullCL, this.conf.exportFile, testPara.user, "sequoiadb", false);
   var errCode = -179;
   handleToolError(args, errCode);

   // 创建导入集合并将大对象导入
   this.otherCl = commCreateCL(db, COMMCSNAME, this.otherCLName);
   args["operation"] = "import";
   args["collection"] = this.conf.impFullCL;
   args["usrname"] = "root";
   args["passwd"] = passwd;
   handleToolError(args, errCode);
}

LobToolTest.prototype.checkResult = function () {
}
lobToolMain(main, testPara);

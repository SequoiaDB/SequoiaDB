/******************************************************************************
*@Description : test sdblobtool import parameter --ignore
*@Modify list :
*               2016-06-24   XueWang Liang  Init
*               覆盖测试用例8794/8795
*******************************************************************************/
import("../lib/lobtool_export_import.js");
LobToolTest.prototype.test = function () {
   var args = buildImportOrExprtArgs("export", this.conf.expFullCL, this.conf.exportFile, this.conf.usr, this.conf.pwd, this.conf.useSsl);
   // 导出大对象到文件exportFile
   execLobToolCommand(args);
   // 创建导入集合并将大对象导入
   this.otherCl = commCreateCL(db, COMMCSNAME, this.otherCLName);
   args["operation"] = "import";
   args["collection"] = this.conf.impFullCL;
   execLobToolCommand(args);

   args["ignorefe"] = true;


   var errCode = -5;
   args["ignorefe"] = false;
   handleToolError(args, errCode);

}
LobToolTest.prototype.checkResult = function () {
}
// Test
lobToolMain(main);

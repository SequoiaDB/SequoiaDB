/******************************************************************************
*@Description : wrong parameter test sdblobtool export
*@Modify list :
*               2016-06-20   XueWang Liang  Init
*               覆盖测试用例8770/8771/8772/8773/8774
******************************************************************************/
import("../lib/lobtool_export_import.js");

LobToolTest.prototype.test = function () {
   var args = buildImportOrExprtArgs("expo", this.conf.expFullCL, this.conf.exportFile);
   var errCode = -6;

   handleToolError(args, errCode);
}

LobToolTest.prototype.checkResult = function () {
}
// Test
lobToolMain(main);

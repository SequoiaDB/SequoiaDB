/******************************************************************************
*@Description : Invalid Argument test sdblobtool migration
*@Modify list :
*               2016-06-24   XueWang Liang  Init
*               覆盖测试用例8812/8813/8814/8815/8816/8817
*******************************************************************************/
import("../lib/lobtool_export_import.js");
LobToolTest.prototype.test = function () {
   // sdblobtool 迁移的选项参数
   var args = buildMigrationArgs(this.conf.srcFullCL, "");

   var errCode = -6;
   // 创建导入集合并将大对象导入
   this.otherCl = commCreateCL(db, COMMCSNAME, this.otherCLName);
   handleToolError(args, errCode);

}
LobToolTest.prototype.checkResult = function () {
}
// Test
lobToolMain(main);


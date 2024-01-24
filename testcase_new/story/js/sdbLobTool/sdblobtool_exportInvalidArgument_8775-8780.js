/******************************************************************************
*@Description : Invalid Argument test sdblobtool export
*@Modify list :
*               2016-06-20   XueWang Liang  Init
*               覆盖测试用例8775/8776/8777/8778/8779/8780
*******************************************************************************/
import("../lib/lobtool_export_import.js");
LobToolTest.prototype.test = function () {
   var parameters = [{ operator: "", clname: this.conf.expFullCL, file: this.conf.exportFile },
   { operator: "export", clname: "", file: this.conf.exportFile },
   { operator: "export", clname: this.conf.expFullCL, file: "" }];

   var hostname = COORDHOSTNAME;
   for (var i = 0; i < parameters.length; ++i) {
      println("loop " + i);
      var args = buildImportOrExprtArgs(parameters[i].operator, parameters[i].clname, parameters[i].file);
      if (parameters[i].hostname === "") {
         args["hostname"] = "";
      }
      var errCode = -6;

      handleToolError(args, errCode);
   }
}

LobToolTest.prototype.checkResult = function () {
}
// Test
lobToolMain(main);




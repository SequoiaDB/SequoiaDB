/******************************************************************************
*@Description : wrong parameter test sdblobtool import
*@Modify list :
*               2016-06-22   XueWang Liang  Init
*               覆盖测试用例8787/8788/8789/8790/8791/8792/8793
******************************************************************************/
import("../lib/lobtool_export_import.js");
function buildFile(filename, content) {
   try {
      var file = new File(filename);
      if (content !== undefined) {
         file.write(content);
      }
      file.close();
   } catch (e) {
      throw new Error(e);
   } finally {
      if (file !== undefined) {
         file.close();
      }
   }
}

LobToolTest.prototype.test = function () {
   try {
      // 测试导入文件不存在时，文件为空时，文件为普通文件时的导入
      var noexistFileName = LocalPath + "/noexist.file";
      var emptyFileName = LocalPath + "/empty.file";
      var normalFileName = LocalPath + "/normal.file";

      buildFile(emptyFileName);
      buildFile(normalFileName, "shskhsaochoacakbckaacvavcvagkcgcdd");
      // 创建导入集合并将大对象导入
      this.otherCl = commCreateCL(db, COMMCSNAME, this.otherCLName);

      var parameters = [{ file: noexistFileName, err: -4 }, { file: emptyFileName, err: -9 }, { file: normalFileName, err: -6 }]
      for (var i = 0; i < parameters.length; ++i) {
         var args = buildImportOrExprtArgs("import", this.conf.impFullCL, parameters[i].file);
         handleToolError(args, parameters[i].err);
      }

   } finally {
      execCommand(cmd, "rm -rf " + emptyFileName);
      execCommand(cmd, "rm -rf " + normalFileName);
   }
}
LobToolTest.prototype.checkResult = function () {
}
// Test
lobToolMain(main);

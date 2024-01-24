/******************************************************************************
 * @Description   : seqDB-17947:用户级配置配置AuditMask掩码
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2019.1.28
 * @LastEditTime  : 2022.08.01
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
// 审计日志需要在日志文件查看，自动化用例不好实现，现自动化只在编目SYSCAT.SYSUSERS查看配置的掩码有写到用户系统表即可
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var user = "user17947";

   try
   {
      // create user
      var auditMask = "!ACCESS|!CLUSTER|!SYSTEM|DML|DDL|DCL|DQL|INSERT|UPDATE|DELETE|OTHER";
      db.createUsr( user, user, { AuditMask: auditMask } );

      // connect catalog, check results
      var cdb = db.getCataRG().getMaster().connect();
      var rc = cdb.SYSAUTH.SYSUSRS.find( { "User": user } );
      var rcAuditMask = rc.current().toObj()["Options"]["AuditMask"];
      rc.close();

      assert.equal( auditMask, rcAuditMask );
   }
   finally 
   {
      db.dropUsr( user, user );
      cdb.close();
   }
}
/******************************************************************************
 * @Description   : seqDB-26432:指定lob oid长度不为24位
 * @Author        : Zhang Yanan
 * @CreateTime    : 2022.04.27
 * @LastEditTime  : 2022.06.10
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_26432";

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var lobPath = WORKDIR + "/lob26432/";

   // lob指定oid为0位
   var oid1 = "";
   checkDifferentLobId( dbcl, oid1, lobPath );

   // lob指定oid为23位
   var oid2 = "000062569a62360004b0a60";
   checkDifferentLobId( dbcl, oid2, lobPath );

   // lob指定oid为25位
   var oid3 = "000062569a62360004b0a6022";
   checkDifferentLobId( dbcl, oid3, lobPath );
}

function checkDifferentLobId ( cl, lobOid, lobPath )
{
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.putLob( lobPath, lobOid );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.getLob( lobOid, lobPath );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.deleteLob( lobOid );
   } );

   // truncateLob接口验证暂时屏蔽，该问题由问题单http://jira.web:8080/browse/SEQUOIADBMAINSTREAM-8392跟踪
   /*assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.truncateLob( lobOid, 0 );
   } );*/
}


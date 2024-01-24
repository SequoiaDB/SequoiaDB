/******************************************************************************
 * @Description   : seqDB-28009 :: 创建监控用户执行内置sql非监控类操作
 * @Author        : Wu Yan
 * @CreateTime    : 2022.09.24
 * @LastEditTime  : 2023.08.07
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

// SEQUOIADBMAINSTREAM-9798
// main( test );
function test ()
{
   try
   {
      var adminUser = "admin28009"
      var userName = "user28009";
      var passwd = "passwd28009";
      db.createUsr( adminUser, passwd, { Role: "admin" } );
      db.createUsr( userName, passwd, { Role: "monitor" } );
      var authDB = new Sdb( COORDHOSTNAME, COORDSVCNAME, userName, passwd );
      var adminDB = new Sdb( COORDHOSTNAME, COORDSVCNAME, adminUser, passwd );

      //ddl/dml/dql
      var csName = "cs28009";
      var clName = "cl28009";
      assert.tryThrow( SDB_NO_PRIVILEGES, function()
      {
         authDB.execUpdate( "create collectionspace " + csName );
      } );

      assert.tryThrow( SDB_NO_PRIVILEGES, function()
      {
         authDB.execUpdate( "create collection " + csName + "." + clName );
      } );

      adminDB.createCS( csName ).createCL( clName );
      assert.tryThrow( SDB_NO_PRIVILEGES, function()
      {
         authDB.execUpdate( "insert into " + csName + "." + clName + "(no) values(1)" );
      } );

      assert.tryThrow( SDB_NO_PRIVILEGES, function()
      {
         authDB.exec( "select * from " + csName + "." + clName );
      } );
   }
   finally
   {
      adminDB.dropUsr( userName, passwd );
      adminDB.dropUsr( adminUser, passwd );
      adminDB.dropCS( csName );
      authDB.close();
      adminDB.close();
   }
}








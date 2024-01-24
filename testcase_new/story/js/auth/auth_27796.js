/******************************************************************************
 * @Description   : seqDB-27796:创建监控用户执行非监控类操作
 * @Author        : Xu Mingxing
 * @CreateTime    : 2022.09.27
 * @LastEditTime  : 2023.08.04
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
// SEQUOIADBMAINSTREAM-9798
// main( test );

function test ()
{
   var userName1 = "user_27796_1";
   var userName2 = "user_27796_2";
   var userName3 = "user_27796_3";
   var csName1 = "csName_27796_1";
   var clName1 = "clName_27796_1";
   var csName2 = "csName_27796_2";
   var clName2 = "clName_27796_2";
   var cl = db.createCS( csName1 ).createCL( clName1 );
   cl.insert( { a: 1 } );
   db.createUsr( userName1, userName1, { Role: "admin" } );
   db.createUsr( userName2, userName2, { Role: "monitor" } );
   try
   {
      // 指定鉴权用户连接coord节点
      var sdb = new Sdb( COORDHOSTNAME, COORDSVCNAME, userName2, userName2 );
      // DDL操作
      assert.tryThrow( SDB_NO_PRIVILEGES, function()
      {
         sdb.createCS( csName2 ).createCL( clName2 );
      } );
      // DML操作
      assert.tryThrow( SDB_NO_PRIVILEGES, function()
      {
         sdb.getCS( csName1 ).getCL( clName1 ).insert( { b: 2 } );
      } );
      // DQL操作
      assert.tryThrow( SDB_NO_PRIVILEGES, function()
      {
         sdb.getCS( csName1 ).getCL( clName1 ).find().toArray();
      } );
      // DCL操作
      assert.tryThrow( SDB_NO_PRIVILEGES, function()
      {
         sdb.createUsr( userName3, userName3 );
      } );
      // 集群管理操作
      assert.tryThrow( SDB_NO_PRIVILEGES, function()
      {
         createAndRemoveNode( sdb );
      } );

      // 指定鉴权用户连接catalog节点
      var sdb = getCatalogConn( db, userName2, userName2 );
      // DDL操作
      assert.tryThrow( SDB_NO_PRIVILEGES, function()
      {
         sdb.createCS( csName2 ).createCL( clName2 );
      } );
      // DML操作
      assert.tryThrow( SDB_NO_PRIVILEGES, function()
      {
         sdb.getCS( "SYSCAT" ).getCL( "SYSCOLLECTIONS" ).insert( { b: 2 } );
      } );
      // DQL操作
      assert.tryThrow( SDB_NO_PRIVILEGES, function()
      {
         sdb.getCS( "SYSCAT" ).getCL( "SYSCOLLECTIONS" ).find().toArray();
      } );

      // 指定鉴权用户连接data节点
      var groupNames = commGetCLGroups( db, csName1 + "." + clName1 );
      var masterNode = db.getRG( groupNames[0] ).getMaster();
      var sdb = new Sdb( masterNode.getHostName(), masterNode.getServiceName(), userName2, userName2 );
      // DDL操作
      assert.tryThrow( SDB_NO_PRIVILEGES, function()
      {
         sdb.createCS( csName2 ).createCL( clName2 );
      } );
      // DML操作
      assert.tryThrow( SDB_NO_PRIVILEGES, function()
      {
         sdb.getCS( csName1 ).getCL( clName1 ).insert( { b: 2 } );
      } );
      // DQL操作
      assert.tryThrow( SDB_NO_PRIVILEGES, function()
      {
         sdb.getCS( csName1 ).getCL( clName1 ).find().toArray();
      } );
   }
   finally
   {
      db.dropUsr( userName2, userName2 );
      db.dropUsr( userName1, userName1 );
      db.dropCS( csName1 );
      sdb.close();
   }
}
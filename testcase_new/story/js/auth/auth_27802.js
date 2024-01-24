/******************************************************************************
 * @Description   : seqDB-27802:重建同名admin用户，更新admin角色为monitor
 * @Author        : Xu Mingxing
 * @CreateTime    : 2022.09.27
 * @LastEditTime  : 2023.08.07
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
// SEQUOIADBMAINSTREAM-9798
// main( test );

function test ()
{
   var userName1 = "user_27802_1";
   var userName2 = "user_27802_2";
   var userName3 = "user_27802_3";
   var csName1 = "csName_27802_1";
   var clName1 = "clName_27802_1";
   var csName2 = "csName_27802_2";
   var clName2 = "clName_27802_2";
   var cl = db.createCS( csName1 ).createCL( clName1 );
   cl.insert( { a: 1 } );
   db.createUsr( userName1, userName1, { Role: "admin" } );
   db.createUsr( userName2, userName2, { Role: "admin" } );
   var sdb = new Sdb( COORDHOSTNAME, COORDSVCNAME, userName1, userName1 );
   sdb.dropUsr( userName1, userName1 );
   db.createUsr( userName1, userName1, { Role: "monitor" } );
   try
   {
      // 指定鉴权用户连接coord节点
      var sdb = new Sdb( COORDHOSTNAME, COORDSVCNAME, userName1, userName1 );
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
      // 监控管理操作
      sdb.list( SDB_LIST_COLLECTIONS ).toString();
      var options = { Role: "monitor" };
      checkListUsers( sdb, userName1, options );
   }
   finally
   {
      db.dropUsr( userName1, userName1 );
      db.dropUsr( userName2, userName2 );
      db.dropCS( csName1 );
      sdb.close();
   }
}
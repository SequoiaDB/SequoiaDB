/******************************************************************************
 * @Description   : seqDB-27804:重建同名monitor用户，更新monitor角色为admin
 * @Author        : Xu Mingxing
 * @CreateTime    : 2022.09.28
 * @LastEditTime  : 2023.08.07
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
// SEQUOIADBMAINSTREAM-9798
// main( test );
function test ()
{
   var userName1 = "user_27804_1";
   var userName2 = "user_27804_2";
   var userName3 = "user_27804_3";
   var csName = "csName_27804";
   var clName = "clName_27804";
   db.createUsr( userName1, userName1, { Role: "admin" } );
   db.createUsr( userName2, userName2, { Role: "monitor" } );
   new Sdb( COORDHOSTNAME, COORDSVCNAME, userName2, userName2 );
   var sdb = new Sdb( COORDHOSTNAME, COORDSVCNAME, userName1, userName1 );
   sdb.dropUsr( userName2, userName2 );
   db.createUsr( userName2, userName2, { Role: "admin" } );
   try
   {
      // 指定鉴权用户连接coord节点
      var sdb = new Sdb( COORDHOSTNAME, COORDSVCNAME, userName2, userName2 );
      // DDL、DML、DQL操作
      DdlAndDmlAndDqlOpr( sdb, csName, clName );
      // DCL操作
      sdb.createUsr( userName3, userName3 );
      sdb.dropUsr( userName3, userName3 );
      // 监控管理操作
      var expResult = [csName + "." + clName];
      checkSnapshotCollection( sdb, expResult );
      sdb.dropCS( csName );
      var options = { Role: "admin" };
      checkListUsers( sdb, userName2, options )
   }
   finally
   {
      db.dropUsr( userName1, userName1 );
      db.dropUsr( userName2, userName2 );
      sdb.close();
   }
}

function DdlAndDmlAndDqlOpr ( sdb, csName, clName )
{
   var cl = commCreateCL( sdb, csName, clName );
   cl.insert( { a: 1 } );
   var countNum = cl.find().count();
   assert.equal( countNum, 1 );
}

function checkSnapshotCollection ( sdb, expResult )
{
   var cursor = sdb.snapshot( SDB_SNAP_COLLECTIONS );
   var tmpArray = [];
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      tmpArray.push( obj.Name );
   }
   assert.equal( isContained( tmpArray, expResult ), true );
   cursor.close();
}
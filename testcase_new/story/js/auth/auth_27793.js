/******************************************************************************
 * @Description   : seqDB-27793:使用默认角色执行操作
 * @Author        : Xu Mingxing
 * @CreateTime    : 2022.09.24
 * @LastEditTime  : 2023.08.07
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
// SEQUOIADBMAINSTREAM-9798
// main( test );

function test ()
{
   var userName1 = "user_27793_1";
   var userName2 = "user_27793_2";
   var csName = "csName_27793";
   var clName = "clName_27793";
   db.createUsr( userName1, userName1 );
   try
   {
      // 指定鉴权用户连接coord节点
      var sdb = new Sdb( COORDHOSTNAME, COORDSVCNAME, userName1, userName1 );
      // DDL、DML、DQL操作
      DdlAndDmlAndDqlOpr( sdb, csName, clName );
      // DCL操作
      sdb.createUsr( userName2, userName2 );
      sdb.dropUsr( userName2, userName2 );
      // 监控管理操作
      var expResult = [csName + "." + clName];
      checkSnapshotCollection( sdb, expResult );
      sdb.dropCS( csName );
      var options = { Role: "admin" };
      checkListUsers( sdb, userName1, options );
      // 集群管理操作
      createAndRemoveNode( sdb );

      // 指定鉴权用户连接catalog节点
      var sdb = getCatalogConn( db, userName1, userName1 );
      // DDL、DML、DQL操作
      DdlAndDmlAndDqlOpr( sdb, csName, clName );
      // 监控管理操作
      checkSnapshotCollection( sdb, expResult );
      sdb.dropCS( csName );

      // 指定鉴权用户连接data节点
      var sdb = getDataConn( db, userName1, userName1 );
      // DDL、DML、DQL操作
      DdlAndDmlAndDqlOpr( sdb, csName, clName );
      // 监控管理操作
      checkSnapshotCollection( sdb, expResult );
      sdb.dropCS( csName );
   }
   finally
   {
      db.dropUsr( userName1, userName1 );
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
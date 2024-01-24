/******************************************************************************
 * @Description   : seqDB-27799:不存在admin用户，创建monitor用户
 *                  seqDB-27800:存在monitor用户，删除admin用户
 *                  seqDB-27801:创建多个admin和monitor用户
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
   var userName1 = "user_27799_1";
   var userName2 = "user_27799_2";
   var userName3 = "user_27799_3";
   var userName4 = "user_27799_4";
   var csName1 = "csName_27799_1";
   var clName1 = "clName_27799_1";
   var csName2 = "csName_27799_2";
   var clName2 = "clName_27799_2";

   // 不存在admin用户，创建monitor用户
   assert.tryThrow( SDB_OPERATION_DENIED, function()
   {
      db.createUsr( userName1, userName1, { Role: "monitor" } );
   } );

   // 存在monitor用户，删除admin用户
   try
   {
      db.createUsr( userName1, userName1, { Role: "admin" } );
      db.createUsr( userName2, userName2, { Role: "monitor" } );
      var sdb = new Sdb( COORDHOSTNAME, COORDSVCNAME, userName1, userName1 );
      assert.tryThrow( SDB_OPERATION_DENIED, function()
      {
         sdb.dropUsr( userName1, userName1 );
      } );
   } finally
   {
      db.dropUsr( userName2, userName2 );
      db.dropUsr( userName1, userName1 );
      sdb.close();
   }

   // 创建多个admin和monitor用户
   try
   {
      var cl = db.createCS( csName1 ).createCL( clName1 );
      cl.insert( { a: 1 } );
      db.createUsr( userName1, userName1, { Role: "admin" } );
      db.createUsr( userName2, userName2, { Role: "admin" } );
      db.createUsr( userName3, userName3, { Role: "monitor" } );
      db.createUsr( userName4, userName4, { Role: "monitor" } );
      // userName1执行数据操作
      var sdb = new Sdb( COORDHOSTNAME, COORDSVCNAME, userName1, userName1 );
      DdlAndDmlAndDqlOpr( sdb, csName2, clName2 );
      // userName1执行监控管理操作
      var expResult = [csName2 + "." + clName2];
      checkSnapshotCollection( sdb, expResult );
      sdb.dropCS( csName2 );
      var options = { Role: "admin" };
      checkListUsers( sdb, userName1, options );

      // userName2执行数据操作
      var sdb = new Sdb( COORDHOSTNAME, COORDSVCNAME, userName2, userName2 );
      DdlAndDmlAndDqlOpr( sdb, csName2, clName2 );
      // userName2执行监控管理操作
      checkSnapshotCollection( sdb, expResult );
      sdb.dropCS( csName2 );
      checkListUsers( sdb, userName2, options );

      // userName3执行数据操作
      var sdb = new Sdb( COORDHOSTNAME, COORDSVCNAME, userName3, userName3 );
      assert.tryThrow( SDB_NO_PRIVILEGES, function()
      {
         sdb.createCS( csName2 ).createCL( clName2 );
      } );
      assert.tryThrow( SDB_NO_PRIVILEGES, function()
      {
         sdb.getCS( csName1 ).getCL( clName1 ).insert( { b: 2 } );
      } );
      assert.tryThrow( SDB_NO_PRIVILEGES, function()
      {
         sdb.getCS( csName1 ).getCL( clName1 ).truncate();
      } );
      assert.tryThrow( SDB_NO_PRIVILEGES, function()
      {
         sdb.getCS( csName1 ).getCL( clName1 ).find().toArray();
      } );
      // userName3执行监控管理操作
      var options = { Role: "monitor" };
      checkListUsers( sdb, userName3, options );

      // userName4执行数据操作
      var sdb = new Sdb( COORDHOSTNAME, COORDSVCNAME, userName4, userName4 );
      assert.tryThrow( SDB_NO_PRIVILEGES, function()
      {
         sdb.createCS( csName2 ).createCL( clName2 );
      } );
      assert.tryThrow( SDB_NO_PRIVILEGES, function()
      {
         sdb.getCS( csName1 ).getCL( clName1 ).insert( { b: 2 } );
      } );
      assert.tryThrow( SDB_NO_PRIVILEGES, function()
      {
         sdb.getCS( csName1 ).getCL( clName1 ).truncate();
      } );
      assert.tryThrow( SDB_NO_PRIVILEGES, function()
      {
         sdb.getCS( csName1 ).getCL( clName1 ).find().toArray();
      } );
      // userName4执行监控管理操作
      checkListUsers( sdb, userName4, options );
   } finally
   {
      db.dropUsr( userName4, userName4 );
      db.dropUsr( userName3, userName3 );
      db.dropUsr( userName2, userName2 );
      db.dropUsr( userName1, userName1 );
      db.dropCS( csName1 );
      sdb.close();
   }
}

function DdlAndDmlAndDqlOpr ( sdb, csName, clName )
{
   var cl = commCreateCL( sdb, csName, clName );
   cl.insert( { a: 1 } );
   cl.truncate();
   var countNum = cl.find().count();
   assert.equal( countNum, 0 );
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
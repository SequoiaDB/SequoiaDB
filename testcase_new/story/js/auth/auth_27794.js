/******************************************************************************
 * @Description   : seqDB-27794 :: 创建管理员用户执行监控类操作 
 * @Author        : Wu Yan
 * @CreateTime    : 2022.09.24
 * @LastEditTime  : 2022.10.08
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   try
   {
      var userName = "user27794";
      var passwd = "passwd27794";
      var role = "admin";
      var sdb = new Sdb( COORDHOSTNAME, COORDSVCNAME );
      sdb.createUsr( userName, passwd, { Role: role } );
      var authDB = new Sdb( COORDHOSTNAME, COORDSVCNAME, userName, passwd );

      //快照操作、list列表操作
      snapshotOpr( authDB );
      listOpr( authDB );

      //执行存储过程
      execProcedure( authDB, userName, passwd );

      //执行sdbcs类/sdbcl类监控
      execSdbCSAndCLOpr( authDB );


      //指定鉴权用户连接catalog节点     
      var cataDB = getCatalogConn( authDB, userName, passwd );
      println( "---cataDB=" + cataDB );
      execSnapshotOpr( cataDB );
      execListOpr( cataDB );
      cataDB.close();

      //选择一个数据节点执行数据操作      
      var dataDB = getDataConn( authDB, userName, passwd );
      println( "---dataDB=" + dataDB );
      execSnapshotOpr( dataDB );
      execListOpr( dataDB );
      dataDB.close();
   }
   finally
   {
      authDB.dropUsr( userName, passwd );
      authDB.close();
   }
}

function execProcedure ( sdb, user, passwd )
{
   try
   {
      //初始化db连接给存储过程使用
      var db = new Sdb( COORDHOSTNAME, COORDSVCNAME, user, passwd );
      var pcdName1 = "createCLOpr";
      var csName = "testpcd27794";
      var clName = "testpcd_cl27794"
      var pcdString1 = "db.createProcedure( function " + pcdName1 + "(" + ") {db.createCS('" + csName + "').createCL('" + clName + "')} )";
      sdb.eval( pcdString1 );
      sdb.eval( pcdName1 + "()" );
      sdb.removeProcedure( pcdName1 );

      var pcdName2 = "listCLPcd27794";
      var pcdString2 = "db.createProcedure( function " + pcdName2 + "(" + ") {db.getCS('" + csName + "').listCollections().toArray()} )";
      sdb.eval( pcdString2 );
      sdb.eval( pcdName2 + "()" );
      sdb.removeProcedure( pcdName2 );
      sdb.dropCS( csName );
   }
   finally
   {
      if( db != null )
      {
         db.close();
      }
   }
}

function execSdbCSAndCLOpr ( sdb )
{
   var domainName = "domain27794";
   var csName = "testcs27794";
   var clName = "testcl27794";
   var groups = commGetGroups( sdb );
   var groupName = groups[0][0].GroupName;
   try
   {
      sdb.createDomain( domainName, [groupName] );
      var cs = sdb.createCS( csName, { Domain: domainName } );
      var cl = cs.createCL( clName );
      var actDomainName = cs.getDomainName();
      assert.equal( actDomainName, domainName );
      cs.listCollections().toArray();
      cl.getDetail().toArray();
   }
   finally
   {
      sdb.dropCS( csName );
      sdb.dropDomain( domainName );
   }
}

function execSnapshotOpr ( sdb )
{
   var snapshotType = [SDB_SNAP_CONTEXTS, SDB_SNAP_CONTEXTS_CURRENT, SDB_SNAP_SESSIONS, SDB_SNAP_SESSIONS_CURRENT, SDB_SNAP_COLLECTIONS,
      SDB_SNAP_COLLECTIONSPACES, SDB_SNAP_DATABASE, SDB_SNAP_SYSTEM, SDB_SNAP_TRANSACTIONS, SDB_SNAP_TRANSACTIONS_CURRENT,
      SDB_SNAP_ACCESSPLANS, SDB_SNAP_HEALTH, SDB_SNAP_CONFIGS, SDB_SNAP_SVCTASKS, SDB_SNAP_QUERIES, SDB_SNAP_LOCKWAITS,
      SDB_SNAP_LATCHWAITS, SDB_SNAP_INDEXSTATS, SDB_SNAP_TRANSWAITS, SDB_SNAP_TRANSDEADLOCK];
   for( var i = 0; i < snapshotType.length; i++ )
   {
      var type = snapshotType[i];
      println( "----snapshot =" + type )
      sdb.snapshot( type ).toArray();
   }
}

function execListOpr ( sdb )
{
   var listType = [SDB_LIST_CONTEXTS, SDB_LIST_CONTEXTS_CURRENT, SDB_LIST_SESSIONS, SDB_LIST_SESSIONS_CURRENT, SDB_LIST_COLLECTIONS, SDB_LIST_COLLECTIONSPACES,
      SDB_LIST_STORAGEUNITS, SDB_LIST_TRANSACTIONS, SDB_LIST_TRANSACTIONS_CURRENT, SDB_LIST_SVCTASKS,
      SDB_LIST_BACKUPS];
   for( var i = 0; i < listType.length; i++ )
   {
      var type = listType[i];
      println( "----list =" + type )
      sdb.list( type ).toArray();
   }
}
/******************************************************************************
 * @Description   : seqDB-28000 :: 创建监控用户执行调用存储过程eval相关监控操作  
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
      var csName = "cs28000";
      var clName = "cl28000";
      var adminUser = "admin28000"
      var userName = "user28000";
      var passwd = "passwd28000";
      var sdb = new Sdb( COORDHOSTNAME, COORDSVCNAME );
      sdb.createUsr( adminUser, passwd, { Role: "admin" } );
      sdb.createUsr( userName, passwd, { Role: "monitor" } );
      var authDB = new Sdb( COORDHOSTNAME, COORDSVCNAME, userName, passwd );
      var adminDB = new Sdb( COORDHOSTNAME, COORDSVCNAME, adminUser, passwd );
      //初始化db连接给存储过程使用
      var db = new Sdb( COORDHOSTNAME, COORDSVCNAME, adminUser, passwd );

      var procedureNames = [];
      //快照操作
      var snapshotPcdName = snapshotPcd( adminDB );
      authDB.eval( snapshotPcdName );
      procedureNames.push( snapshotPcdName );

      //列表操作
      var listPcdName = listPcd( adminDB );
      authDB.eval( listPcdName );
      procedureNames.push( listPcdName );

      //获取元数据对象   
      adminDB.createCS( csName ).createCL( clName );
      var pcdName1 = "getCSPcd";
      var pcdString1 = "db.createProcedure( function " + pcdName1 + "(" + ") {db.getCS('" + csName + "').getCL('" + clName + "')} )";
      adminDB.eval( pcdString1 );
      procedureNames.push( pcdName1 );
      authDB.eval( pcdName1 + "()" );

      var pcdName2 = "sessionAttrPcd";
      var pcdString2 = "db.createProcedure( function " + pcdName2 + "() {db.setSessionAttr( { PreferredInstance: 's' } );db.getSessionAttr()} )";
      adminDB.eval( pcdString2 );
      procedureNames.push( pcdName2 );
      authDB.eval( pcdName2 + "()" );

      var pcdName3 = "listCLPcd";
      var pcdString3 = "db.createProcedure( function " + pcdName3 + "(" + ") {db.getCS('" + csName + "').listCollections().toArray()} )";
      adminDB.eval( pcdString3 );
      procedureNames.push( pcdName3 );
      authDB.eval( pcdName3 + "()" );

      var pcdName4 = "getCLPcd";
      var pcdString4 = "db.createProcedure( function " + pcdName4 + "(" + ") {db.getCS('" + csName + "').getCL('" + clName + "').getDetail().toArray();"
         + "db.getCS('" + csName + "').getCL('" + clName + "').count()} )";
      adminDB.eval( pcdString4 );
      procedureNames.push( pcdName4 );
      authDB.eval( pcdName4 + "()" );


      //查询类SdbQuery.count
      var pcdName5 = "getCountPcd";
      var pcdString5 = "db.createProcedure( function " + pcdName5 + "(" + ") {db.getCS('" + csName + "').getCL('" + clName + "').find().count()} )";
      adminDB.eval( pcdString5 );
      procedureNames.push( pcdName5 );
      authDB.eval( pcdName5 + "()" );
      adminDB.dropCS( csName );


      //获取组和节点元数据信息
      var groups = commGetGroups( adminDB );
      var groupName = groups[0][0].GroupName;
      var pcdName6 = "mgrPcdName";
      var pcdString6 = "db.createProcedure( function " + pcdName6 + "(" + ") {db.getRG('" + groupName + "').getMaster().connect()} )";
      adminDB.eval( pcdString6 );
      procedureNames.push( pcdName6 );
      authDB.eval( pcdName6 + "()" );
   }
   finally
   {
      adminDB.dropUsr( userName, passwd );
      adminDB.dropUsr( adminUser, passwd );
      clearProcedures( adminDB, procedureNames );
      authDB.close();
      adminDB.close();
      db.close();
      sdb.close()
   }
}

function snapshotPcd ( db )
{
   db.createProcedure(
      function execSnapshot ()
      {
         var snapshotType = [SDB_SNAP_CONTEXTS, SDB_SNAP_CONTEXTS_CURRENT, SDB_SNAP_SESSIONS, SDB_SNAP_SESSIONS_CURRENT, SDB_SNAP_COLLECTIONS,
            SDB_SNAP_COLLECTIONSPACES, SDB_SNAP_DATABASE, SDB_SNAP_SYSTEM, SDB_SNAP_CATALOG, SDB_SNAP_TRANSACTIONS, SDB_SNAP_TRANSACTIONS_CURRENT,
            SDB_SNAP_ACCESSPLANS, SDB_SNAP_HEALTH, SDB_SNAP_CONFIGS, SDB_SNAP_SVCTASKS, SDB_SNAP_SEQUENCES, SDB_SNAP_QUERIES, SDB_SNAP_LOCKWAITS,
            SDB_SNAP_LATCHWAITS, SDB_SNAP_INDEXSTATS, SDB_SNAP_TRANSWAITS, SDB_SNAP_TRANSDEADLOCK];
         for( var i = 0; i < snapshotType.length; i++ )
         {
            var type = snapshotType[i];
            db.snapshot( type ).toArray();
         }
      }
   );
   var procedureName = "execSnapshot";
   return procedureName;
}

function listPcd ( db )
{
   db.createProcedure(
      function listOpr ()
      {
         var listType = [SDB_LIST_CONTEXTS, SDB_LIST_CONTEXTS_CURRENT, SDB_LIST_SESSIONS, SDB_LIST_SESSIONS_CURRENT, SDB_LIST_COLLECTIONS, SDB_LIST_COLLECTIONSPACES,
            SDB_LIST_STORAGEUNITS, SDB_LIST_GROUPS, SDB_LIST_TASKS, SDB_LIST_TRANSACTIONS, SDB_LIST_TRANSACTIONS_CURRENT, SDB_LIST_SVCTASKS, SDB_LIST_SEQUENCES,
            SDB_LIST_USERS, SDB_LIST_BACKUPS, SDB_LIST_DATASOURCES];
         for( var i = 0; i < listType.length; i++ )
         {
            var type = listType[i];
            db.list( type ).toArray();
         }
      }
   )
   var procedureName = "listOpr";
   return procedureName;
}

function clearProcedures ( sdb, procedureNames )
{
   var procedureNum = procedureNames.length;
   if( procedureNum !== 0 )
   {
      for( var i = 0; i < procedureNum; i++ )
      {
         var name = procedureNames[i];
         sdb.removeProcedure( name );
      }
   }
}








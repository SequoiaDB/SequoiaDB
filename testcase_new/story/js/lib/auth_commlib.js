import( "../lib/basic_operation/commlib.js" );
import( "../lib/main.js" );

/*******************************************************************************
@Description : 执行所有查看快照操作
@Modify 
*******************************************************************************/
function snapshotOpr ( sdb )
{
   var snapshotType = [SDB_SNAP_CONTEXTS, SDB_SNAP_CONTEXTS_CURRENT, SDB_SNAP_SESSIONS, SDB_SNAP_SESSIONS_CURRENT, SDB_SNAP_COLLECTIONS,
      SDB_SNAP_COLLECTIONSPACES, SDB_SNAP_DATABASE, SDB_SNAP_SYSTEM, SDB_SNAP_CATALOG, SDB_SNAP_TRANSACTIONS, SDB_SNAP_TRANSACTIONS_CURRENT,
      SDB_SNAP_ACCESSPLANS, SDB_SNAP_HEALTH, SDB_SNAP_CONFIGS, SDB_SNAP_SVCTASKS, SDB_SNAP_SEQUENCES, SDB_SNAP_QUERIES, SDB_SNAP_LOCKWAITS,
      SDB_SNAP_LATCHWAITS, SDB_SNAP_INDEXSTATS, SDB_SNAP_TRANSWAITS, SDB_SNAP_TRANSDEADLOCK];
   for( var i = 0; i < snapshotType.length; i++ )
   {
      var type = snapshotType[i];
      sdb.snapshot( type ).toArray();
   }
}

/*******************************************************************************
@Description : 非coord节点执行所有查看快照操作
@Modify 
*******************************************************************************/
function snapshotOprForOtherNode ( sdb )
{
   var snapshotType = [SDB_SNAP_CONTEXTS, SDB_SNAP_CONTEXTS_CURRENT, SDB_SNAP_SESSIONS, SDB_SNAP_SESSIONS_CURRENT, SDB_SNAP_COLLECTIONS,
      SDB_SNAP_COLLECTIONSPACES, SDB_SNAP_DATABASE, SDB_SNAP_SYSTEM, SDB_SNAP_TRANSACTIONS, SDB_SNAP_TRANSACTIONS_CURRENT,
      SDB_SNAP_ACCESSPLANS, SDB_SNAP_HEALTH, SDB_SNAP_CONFIGS, SDB_SNAP_SVCTASKS, SDB_SNAP_QUERIES,
      SDB_SNAP_LOCKWAITS, SDB_SNAP_LATCHWAITS, SDB_SNAP_INDEXSTATS, SDB_SNAP_TRANSWAITS, SDB_SNAP_TRANSDEADLOCK];
   for( var i = 0; i < snapshotType.length; i++ )
   {
      var type = snapshotType[i];
      sdb.snapshot( type ).toArray();
   }
}

/*******************************************************************************
@Description : 执行所有查看list操作
@Modify 
*******************************************************************************/
function listOpr ( sdb )
{
   var listType = [SDB_LIST_CONTEXTS, SDB_LIST_CONTEXTS_CURRENT, SDB_LIST_SESSIONS, SDB_LIST_SESSIONS_CURRENT, SDB_LIST_COLLECTIONS, SDB_LIST_COLLECTIONSPACES,
      SDB_LIST_STORAGEUNITS, SDB_LIST_GROUPS, SDB_LIST_TASKS, SDB_LIST_TRANSACTIONS, SDB_LIST_TRANSACTIONS_CURRENT, SDB_LIST_SVCTASKS, SDB_LIST_SEQUENCES,
      SDB_LIST_USERS, SDB_LIST_BACKUPS, SDB_LIST_DATASOURCES];
   for( var i = 0; i < listType.length; i++ )
   {
      var type = listType[i];
      sdb.list( type ).toArray();
   }
}

/*******************************************************************************
@Description : 非coord节点执行所有查看list操作
@Modify 
*******************************************************************************/
function listOprForOtherNode ( sdb )
{
   var listType = [SDB_LIST_CONTEXTS, SDB_LIST_CONTEXTS_CURRENT, SDB_LIST_SESSIONS, SDB_LIST_SESSIONS_CURRENT, SDB_LIST_COLLECTIONS,
      SDB_LIST_COLLECTIONSPACES, SDB_LIST_STORAGEUNITS, SDB_LIST_TRANSACTIONS, SDB_LIST_TRANSACTIONS_CURRENT, SDB_LIST_SVCTASKS, SDB_LIST_BACKUPS];
   for( var i = 0; i < listType.length; i++ )
   {
      var type = listType[i];
      sdb.list( type ).toArray();
   }
}

/*******************************************************************************
@Description : 执行所有查看list操作
@Modify 
*******************************************************************************/
function createAndRemoveNode ( sdb )
{
   var groups = commGetGroups( sdb );
   var groupName = groups[0][0].GroupName;
   var rg = sdb.getRG( groupName );
   var hostName = groups[0][1].HostName;
   var svc = parseInt( RSRVPORTBEGIN ) + 40;
   var dbpath = RSRVNODEDIR + "data/" + svc;
   rg.createNode( hostName, svc, dbpath );
   rg.start();
   rg.removeNode( hostName, svc );

}

/*******************************************************************************
@Description : 检查list用户列表中用户名对应角色信息
@Modify 
*******************************************************************************/
function checkListUsers ( db, user, options )
{
   var cursor = db.list( SDB_LIST_USERS, { "User": user } );
   var count = 0;
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      assert.equal( obj.Options, options );
      count++;
   }

   assert.equal( count, 1 );
}

/*******************************************************************************
@Description : 检查list用户列表中用户名对应角色信息
@Modify 
*******************************************************************************/
function ddlAndDmlAndDqlOpr ( sdb, csName, clName )
{
   var dbcl = commCreateCL( sdb, csName, clName );
   dbcl.insert( { a: 1 } );
   var countNum = dbcl.find().count();
   assert.equal( countNum, 1 );
   sdb.dropCS( csName );
}

/*******************************************************************************
@Description : 选择一个编目节点建立连接
@Modify 
*******************************************************************************/
function getCatalogConn ( sdb, userName, passwd )
{
   var cataInfo = sdb.getRG( "SYSCatalogGroup" ).getDetail();
   var hostName = "";
   var svcName = "";
   while( cataInfo.next() )
   {
      var obj = cataInfo.current().toObj().Group;
      hostName = obj[0].HostName;
      svcName = obj[0].Service[0].Name;
   }
   cataInfo.close();
   var cataDB = new Sdb( hostName, svcName, userName, passwd );
   return cataDB;
}

/*******************************************************************************
@Description : 选择一个数据节点建立连接
@Modify 
*******************************************************************************/
function getDataConn ( sdb, userName, passwd )
{
   var groups = commGetGroups( sdb );
   var hostName = groups[0][1].HostName;
   var svcName = groups[0][1].svcname;
   var dataConn = new Sdb( hostName, svcName, userName, passwd );
   return dataConn;
}


/*******************************************************************************
@Description : 获取数据组名
@Modify list : 2019-11-18 zhao xiaoni init
*******************************************************************************/
function getDataGroupNames ()
{
   var groups = commGetGroups( db, false, "", false, false, false );
   var dataGroupNames = [];
   for( var i = 0; i < groups.length; i++ )
   {
      var tmpArray = groups[i];
      if( tmpArray[0].GroupName !== "SYSCatalogGroup" && tmpArray[0].GroupName !== "SYSCoord" )
      {
         dataGroupNames.push( tmpArray[0].GroupName );
      }
   }
   return dataGroupNames;
}

function isContained ( actResult, expResult )
{
   var flag = true;
   for( var i = 0; i < expResult.length; i++ )
   {
      if( actResult.indexOf( expResult[i] ) == -1 )
      {
         flag = false;
         break;
      }
   }
   return flag;
}

/************************************
*@Description: try to drop user by name
*@author:      tangtao
*@createDate:  2022/09/26
**************************************/
function cleanUsers ( user )
{
   try
   {
      db.dropUsr( user, user );
   }
   catch( e )
   {
      if( e != SDB_AUTH_USER_NOT_EXIST )
      {
         throw new Error( e );
      }
   }
}

/************************************
*@Description: try to drop role by name
*@author:      tangtao
*@createDate:  2023/09/05
**************************************/
function cleanRole ( roleName )
{
   try
   {
      db.dropRole( roleName );
   }
   catch( e )
   {
      if( e != SDB_AUTH_ROLE_NOT_EXIST )
      {
         throw new Error( e );
      }
   }
}

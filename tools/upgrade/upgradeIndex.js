/****************************************************************
@decription:   Upgrade index

@input:        hostname:   String, eg: "localhost", required
               svcname:    Number, eg: 11810, required
               username:   String, required
               password:   String, default: ""
               cipherfile: String
               token:      String
               checkonly:  Boolean, check only, don't do upgrade, required

@example:
    ../../bin/sdb -f upgradeIndex.js -e 'var hostname="localhost";
                                         var svcname=11810;var checkonly=true;'

@author:       Ting YU 2021-03-23
****************************************************************/

var HOSTNAME = "" ;
var SVCNAME = "" ;
var USERNAME = "" ;
var PASSWD = "" ;
var CIPHER_FILE = "" ;
var TOKEN = "" ;
var CHECKONLY = "" ;

// check parameter
if ( typeof( hostname ) === "undefined" )
{
   throw new Error( "no parameter [hostname] specified" ) ;
}
else if( hostname.constructor !== String )
{
   throw new Error( "Invalid para[hostname], should be String" ) ;
}
HOSTNAME = hostname ;

if ( typeof( svcname ) === "undefined" )
{
   throw new Error( "no parameter [svcname] specified" ) ;
}
else if( svcname.constructor !== Number )
{
   throw new Error( "Invalid para[svcname], should be Number" ) ;
}
SVCNAME = svcname ;

if ( typeof( username ) === "undefined" )
{
   throw new Error( "no parameter [username] specified" ) ;
}
else if( username.constructor !== String )
{
   throw new Error( "Invalid para[username], should be String" ) ;
}
USERNAME = username ;

if ( typeof( cipherfile ) !== "undefined" )
{
   if( cipherfile.constructor !== String )
   {
      throw new Error( "Invalid para[cipherfile], should be String" ) ;
   }
   CIPHER_FILE = cipherfile ;

   if ( typeof( token ) !== "undefined" &&
        token.constructor !== String )
   {
      throw new Error( "Invalid para[token], should be String" ) ;
   }
   TOKEN = token ;
}
else 
{
   if ( typeof( password ) === "undefined" )
   {
      throw new Error( "no parameter [password] specified" ) ;
   }
   else if( password.constructor !== String )
   {
      throw new Error( "Invalid para[password], should be String" ) ;
   }
   PASSWD = password ;
}

if ( typeof( checkonly ) === "undefined" )
{
   throw new Error( "no parameter [checkonly] specified" ) ;
}
else if( checkonly.constructor !== Boolean )
{
   throw new Error( "Invalid para[checkonly], should be Boolean" ) ;
}
CHECKONLY = checkonly ;

/*************** main entry *****************/

const IDX_TYPE_UNKNOWN     = 0 ;
const IDX_TYPE_CONSISTENT  = 1 ;
const IDX_TYPE_STANDALONE  = 2 ;
const IDX_TYPE_CAN_UPGRADE = 3 ;
const IDX_TYPE_MISSING     = 4 ;
const IDX_TYPE_CONFLICT    = 5 ;

const FIELD_ID          = "ID" ;
const FIELD_COLLECTION  = "Collection" ;
const FIELD_INDEXNAME   = "IndexName" ;
const FIELD_INDEXTYPE   = "IndexType" ;
const FIELD_INDEXATTR   = "IndexAttr" ;
const FIELD_INDEXKEY    = "IndexKey" ;
const FIELD_REASON      = "Reason" ;
const FIELD_RESULT      = "Result" ;
const FIELD_RESULTCODE  = "ResultCode" ;
const FIELD_GROUPNAME   = "GroupName" ;
const FIELD_NODENAME    = "NodeName" ;
const FIELD_MISSING     = "Missing" ;
const FIELD_LOCAL_CL    = "Local CL" ;
const FIELD_AUTOINDEXID = "AutoIndexId=false" ;
const FIELD_ENSURESHARD = "EnsureShardingIndex=false" ;

const MASK_CLATTR_NOIDIDX = 0x02 ;

var GROUP_NODES_MAP = new UtilMap() ; // < groupName, nodeList >
var CATALOG_NODE_LIST = [] ;

var NONEED_UPGRADE_FMT = new NoNeedUpgradeFormator() ;
var CAN_UPGRADE_FMT = new CanUpgradeFormator() ;
var CANNOT_UPGRADE_FMT = new CannotUpgradeFormator() ;
var DETAIL_FMT = new DetailFormator() ;
var UPGRADE_RESULT_FMT = new UpgradeResultFormator() ;
var SUGGEST_FMT = new SuggestionFormator() ;
var TOTAL_FMT = new TotalCntFormator() ;

var UPGRADE_CLUNIT_LIST = [] ;
var SUBCLUNIT_LIST = [] ;
var UPGRADE_MAINCLUNIT_LIST = [] ;
var USER = null ;

try
{
   if ( CIPHER_FILE == "" )
   {
      USER = new User( USERNAME, PASSWD );
      var db = new Sdb( HOSTNAME, SVCNAME, USERNAME, PASSWD ) ;
   }
   else
   {
      if ( CIPHER_FILE == "~/sequoiadb/passwd" )
      {
         USER = new CipherUser(USERNAME).token(TOKEN) ;
      }
      else
      {
         USER = new CipherUser(USERNAME).token(TOKEN).cipherFile(CIPHER_FILE) ;
      }
      var db = new Sdb( HOSTNAME, SVCNAME, USER ) ;
   }
}
catch( e )
{
   println( "Failed to connect coord[" + HOSTNAME + ":" + SVCNAME + 
            "], username[" + USERNAME + "], error[" + e + "]" ) ;
   throw new Error() ;
}
try
{
   check() ;
   NONEED_UPGRADE_FMT.print() ;
   CAN_UPGRADE_FMT.print() ;
   CANNOT_UPGRADE_FMT.print() ;
   DETAIL_FMT.print() ;
   TOTAL_FMT.set( NONEED_UPGRADE_FMT.count(),
                  CAN_UPGRADE_FMT.count(),
                  CANNOT_UPGRADE_FMT.count() ) ;

   if ( !CHECKONLY )
   {
      execute() ;
      UPGRADE_RESULT_FMT.print() ;
      TOTAL_FMT.setUpgradeInfo( UPGRADE_RESULT_FMT.succCnt(),
                                UPGRADE_RESULT_FMT.failCnt() ) ;
   }

   SUGGEST_FMT.print() ;
   TOTAL_FMT.print() ;
}
catch( e )
{
   if( e instanceof Error )
   {
      print( "Error Stack:\n" + e.stack ) ;
   }
   throw e ;
}

function getNodeInfo()
{
   var rc = db.list( SDB_LIST_GROUPS ) ;
   while ( rc.next() )
   {
      var obj = rc.current().toObj() ;
      var groupName = obj.GroupName ;

      if ( groupName == "SYSCoord" )
      {
         continue ;
      }

      var nodeList = [] ;
      var groupList = obj.Group ;
      for ( var i in groupList )
      {
         var hostname = groupList[i].HostName ;
         var svcname = groupList[i].Service[0].Name ;
         var nodename = hostname + ":" + svcname ; // eg: hostname1:11810
         nodeList.push( nodename ) ;
      }
      if ( groupName == "SYSCatalogGroup" )
      {
         CATALOG_NODE_LIST = nodeList.concat() ;
      }
      else
      {
         GROUP_NODES_MAP.add( groupName, nodeList ) ;
      }
   }
}

// clusterCLList formate:
// [
//   { Collection: 'foo.bar', GroupNameList: ['db1','db2'],
//     AutoIndexId: false, EnsureShardingIndex: false, MainCLName: null },
//   ...
// ]
// mainCLList formate:
// [
//   { Collection: 'foo.maincl', ShardingKey: {a:1},
//     SubCLList: ['foo.subcl1',...] },
//   ...
// ]
// localCLList formate:
// [
//   { Collection: 'foo.bar', NodeNameList: ['host1:20000',...] },
//   ...
// ]
function getCollectionInfo( clusterCLList, mainCLList, localCLList )
{
   // check split task before
   var rc = db.listTasks( { TaskType: 0, Status: { $ne: 9 } } ) ;
   if ( rc.next() )
   {
      println( "There are some split tasks, which may affect check result" ) ;
      throw new Error() ;
   }

   // get collection from catalog
   var clusterCLMap = new UtilMap() ; // < cl name, group name list >
   var rc = db.snapshot( SDB_SNAP_CATALOG ) ;
   while ( rc.next() )
   {
      var rcObj = rc.current().toObj() ;
      var cataInfoObj = rcObj.CataInfo ;
      var clName = rcObj.Name ;

      if ( rcObj.DataSourceID != undefined )
      {
         // it is data source collection, just ignore it
         continue ;
      }   
      if ( rcObj.IsMainCL )
      {
         // it is main collection
         var subCLNameList = [] ;
         for ( var i in cataInfoObj )
         {
            var obj = cataInfoObj[i] ;
            subCLNameList.push( obj.SubCLName ) ;
         }
         mainCLList.push( { Collection: clName, ShardingKey: rcObj.ShardingKey,
                            SubCLList: subCLNameList } ) ;
      }
      else
      {
         var groupList = [] ;
         for ( var i in cataInfoObj )
         {
            var obj = cataInfoObj[i] ;
            var groupname = obj.GroupName ;
            groupList.push( groupname ) ;
         }

         var ensureShard = false ;
         if ( true == rcObj.EnsureShardingIndex )
         {
            ensureShard = true ;
         }
         var autoId = true ;
         if ( rcObj.Attribute & MASK_CLATTR_NOIDIDX )
         {
            autoId = false ;
         }
         var mainclName = null ;
         if ( rcObj.MainCLName != undefined )
         {
            mainclName = rcObj.MainCLName ;
         }

         clusterCLList.push( { Collection: clName,
                               GroupNameList: groupList,
                               AutoIndexId: autoId,
                               EnsureShardingIndex: ensureShard,
                               MainCLName: mainclName } ) ;
         clusterCLMap.add( clName, groupList ) ;
      }
   }

   // get collection from data
   var rc = db.snapshot( SDB_SNAP_COLLECTIONS ) ;
   while ( rc.next() )
   {
      var rcObj = rc.current().toObj() ;
      var collection = rcObj.Name ;
      var detailObj = rcObj.Details ;
      var nodesInLocal = [] ;
      var groupsInCata = clusterCLMap.get( collection ) ;
      for ( var i in detailObj )
      {
         var obj = detailObj[i] ;
         var groupname = obj.GroupName ;
         if ( null == groupsInCata ||
              -1 == groupsInCata.indexOf( groupname ) )
         {
            var groupObj = obj.Group ;
            for ( var j in groupObj )
            {
               var nodename = groupObj[j].NodeName ;
               nodesInLocal.push( groupObj[j].NodeName ) ;
            }
         }
      }
      if ( nodesInLocal.length > 0 )
      {
         localCLList.push( { Collection: collection,
                             NodeNameList: nodesInLocal } ) ;
      }
   }
}

function check()
{
   getNodeInfo() ;

   // clusterCLList formate:
   // [
   //   { Collection: 'foo.bar', GroupNameList: ['db1','db2'],
   //     AutoIndexId: false, EnsureShardingIndex: false }, MainCLName: null
   //   ...
   // ]
   var clusterCLList = [] ;
   // mainCLList formate:
   // [
   //   { Collection: 'foo.maincl', ShardingKey: {a:1},
   //     SubCLList: ['foo.subcl1',...] },
   //   ...
   // ]
   var mainCLList = [] ;
   // localCLList formate:
   // [
   //   { Collection: 'foo.bar', NodeNameList: ['host1:20000',...] },
   //   ...
   // ]
   var localCLList = [] ;

   getCollectionInfo( clusterCLList, mainCLList, localCLList ) ;

   for ( var i in clusterCLList )
   {
      var cl = clusterCLList[i] ;
      checkClusterCL( cl.Collection, cl.GroupNameList, cl.AutoIndexId, 
                      cl.EnsureShardingIndex, cl.MainCLName ) ;
   }

   for ( var i in mainCLList )
   {
      var cl = mainCLList[i] ;
      checkMainCL( cl.Collection, cl.ShardingKey, cl.SubCLList ) ;
   }

   for ( var i in localCLList )
   {
      var cl = localCLList[i] ;
      checkLocalCL( cl.Collection, cl.NodeNameList ) ;
   }
}

function checkClusterCL( clName, groupNameList, autoIdxId, ensureShardingIdx, 
                         mainCLName )
{
   var csName      = clName.split( "." )[0] ;
   var clShortName = clName.split( "." )[1] ;
   var collection  = db.getCS( csName ).getCL( clShortName ) ;

   // build cataObjList, format:
   // [
   //   { GroupName: "group1", NodeNameList: [ "hostname1:11810", ... ] },
   //   ...
   // ]
   var cataObjList = [] ;
   for ( var i in groupNameList )
   {
      var groupName = groupNameList[i] ;
      var nodeList = GROUP_NODES_MAP.get( groupName ) ;
      cataObjList.push( { GroupName: groupName, NodeNameList: nodeList } ) ;
   }

   var clUnit = new ClusterCLUnit( clName, cataObjList, autoIdxId, 
                                   ensureShardingIdx, mainCLName ) ;

   // loop indexes in catalog
   var rc = collection.listIndexes() ;
   while ( rc.next() )
   {
      var rcObj = rc.current().toObj() ;
      clUnit.addByCatalog( rcObj.IndexDef ) ;
   }

   // loop indexes in all data nodes
   var lastDef = {} ;
   var lastGroupName = "" ;
   var hasLast = false ;
   var nodeObjList = [] ;  // format:
                           // [
                           //   { NodeName: "hostname1:11810", UniqueID: 123 },
                           //   ...
                           // ]
   // get indexes from data, sort by IndexDef and GroupName
   var rc = collection.snapshotIndexes( { RawData: true }, {},
                                        { "IndexDef.name": 1,
                                          "IndexDef.key": 1,
                                          "IndexDef.unique": 1,
                                          "IndexDef.dropDups": 1,
                                          "IndexDef.enforced": 1,
                                          "IndexDef.NotNull": 1,
                                          "IndexDef.NotArray": 1,
                                          "IndexDef.Global": 1,
                                          "GroupName": 1 } ) ;
   while ( rc.next() )
   {
      var rcObj = rc.current().toObj() ;
      var def = rcObj.IndexDef ;
      if ( def == undefined )
      {
         continue ;
      }
      var uniqueID = def.UniqueID ;
      var groupname = rcObj.GroupName ;
      var nodename = rcObj.NodeName ;
      var nodeObj = { NodeName: nodename, UniqueID: uniqueID } ;

      if ( !hasLast )
      {
         lastDef = def ;
         lastGroupName = groupname ;
         hasLast = true ;
         nodeObjList.push( nodeObj ) ;
         continue ;
      }
      if ( isEqualIdx( lastDef, def ) )
      {
         if ( lastGroupName == groupname )
         {
            nodeObjList.push( nodeObj ) ;
         }
         else
         {
            clUnit.setByData( lastDef, lastGroupName, nodeObjList ) ;
            nodeObjList = [] ;
            nodeObjList.push( nodeObj ) ;
            lastGroupName = groupname ;
         }
      }
      else
      {
         clUnit.setByData( lastDef, lastGroupName, nodeObjList ) ;
         nodeObjList = [] ;
         nodeObjList.push( nodeObj ) ;
         lastDef = def ;
         lastGroupName = groupname ;
      }
   }
   if ( nodeObjList.length > 0 )
   {
      clUnit.setByData( def, groupname, nodeObjList ) ;
   }

   // format
   clUnit.format( NONEED_UPGRADE_FMT,
                  CAN_UPGRADE_FMT,
                  CANNOT_UPGRADE_FMT,
                  DETAIL_FMT ) ;

   if ( clUnit.canUpgradeCnt() > 0 )
   {
      UPGRADE_CLUNIT_LIST.push( clUnit ) ;
   }
   if ( clUnit.mainCLName() != null )
   {
      SUBCLUNIT_LIST.push( clUnit ) ;
   }
}

function checkMainCL( clName, shardingKey, subCLNameList )
{
   if ( subCLNameList.length == 0 )
   {
      // it is empty main-collection
      return ;
   }

   // find the sub-collections, and find the sub-collections with 
   // least number of indexes
   var subCLUnitList = [] ;
   var posOfLeastIdxCL = 0 ;
   for ( var i in SUBCLUNIT_LIST )
   {
      var clUnit = SUBCLUNIT_LIST[i] ;
      if ( clUnit.mainCLName() == clName )
      {
         subCLUnitList.push( clUnit ) ;
         if ( clUnit.idxUnitList.length < 
              subCLUnitList[posOfLeastIdxCL].idxUnitList.length )
         {
            // mark the position of the sub-collection with least
            // number of indexes in subCLUnitList
            posOfLeastIdxCL = subCLUnitList.length - 1 ;
         }
      }
      if ( subCLUnitList.length == subCLNameList.length )
      {
         // found out all sub-collection
         break ;
      }
   }

   if ( subCLUnitList.length < subCLNameList.length )
   {
      // some sub-collections are missing
      return ;
   }

   var mainCLUnit = new MainCLUnit( clName, shardingKey ) ;

   // loop indexes in catalog
   var csName      = clName.split( "." )[0] ;
   var clShortName = clName.split( "." )[1] ;
   var collection  = db.getCS( csName ).getCL( clShortName ) ;
   var rc = collection.listIndexes() ;
   while ( rc.next() )
   {
      mainCLUnit.addByCatalog( rc.current().toObj().IndexDef ) ;
   }

   // loop the sub-collection's every indexes
   var leastIdxCLUnit = subCLUnitList[posOfLeastIdxCL] ;
   for ( var i in leastIdxCLUnit.idxUnitList )
   {
      var idxUnit = leastIdxCLUnit.idxUnitList[i] ;
      var idxDef = idxUnit.getIdxDef() ;
      var allSubCLHas = true ;
      
      if ( idxUnit.getIdxName() == '$id' || idxUnit.getIdxName()  == '$shard' )
      {
         // main-collection doesn't have system index
         continue ;
      }
      if ( idxUnit.getUpgradeType() != IDX_TYPE_CAN_UPGRADE &&
           idxUnit.getUpgradeType() != IDX_TYPE_CONSISTENT )
      {
         // only check consistent index and index can be upgrade to consistent
         continue ;
      }
      
      // loop every sub-collection, find the same index
      for ( var j in subCLUnitList )
      {
         var clUnit = subCLUnitList[j] ;
         var foundOut = false ;
         if ( j == posOfLeastIdxCL )
         {
            foundOut = true ;
            continue ;
         }
         for ( var k in clUnit.idxUnitList )
         {
            var idxUnit1 = clUnit.idxUnitList[k] ;
            if ( ( idxUnit1.getUpgradeType() == IDX_TYPE_CAN_UPGRADE ||
                   idxUnit1.getUpgradeType() == IDX_TYPE_CONSISTENT ) && 
                 idxUnit1.is( idxDef ) )
            {
               foundOut = true ;
               break ;
            }
         }
         if ( !foundOut )
         {
            // this sub-collection doesn't find out the index
            allSubCLHas = false ;
            break ;
         }
      }
      if ( allSubCLHas )
      {
         mainCLUnit.setBySubcl( idxDef ) ;
      }
   }

   if ( mainCLUnit.canUpgradeCnt() > 0 )
   {
      // format
      for ( var i in mainCLUnit.canUpgradeIdxDefList )
      {
         var def = mainCLUnit.canUpgradeIdxDefList[i] ;
         CAN_UPGRADE_FMT.push( clName,
                               def.name,
                               JSON.stringify( def.key ),
                               getIdxAttrDesc( def ),
                               "Consistent" ) ;
      }
      
      UPGRADE_MAINCLUNIT_LIST.push( mainCLUnit ) ;
   }
   // format: no need to upgrade index
   for ( var i in mainCLUnit.catIdxDefList )
   {
      var def = mainCLUnit.catIdxDefList[i] ;
      NONEED_UPGRADE_FMT.push( clName, def.name, "Consistent" ) ;
   }

}

function checkLocalCL( clName, nodeNameList )
{
   var csName      = clName.split( "." )[0] ;
   var clShortName = clName.split( "." )[1] ;

   var clUnit = new LocalCLUnit( clName, nodeNameList ) ;

   // get every data-node's indexes
   for ( var i in nodeNameList )
   {
      var arr = nodeNameList[i].split( ":" ) ;
      var hostname = arr[0] ;
      var svcname = arr[1] ;
      var dataDB = new Sdb( hostname, svcname, USER ) ;

      var collection = dataDB.getCS( csName ).getCL( clShortName ) ;

      var rc = collection.listIndexes() ;
      while ( rc.next() )
      {
         var rcObj = rc.current().toObj() ;
         clUnit.set( rcObj.IndexDef ) ;
      }
   }

   clUnit.format( CANNOT_UPGRADE_FMT, DETAIL_FMT ) ;
}

function execute()
{
   upgradeSysCollection() ;

   for ( var i in UPGRADE_CLUNIT_LIST )
   {
      var clUnit = UPGRADE_CLUNIT_LIST[i] ;
      for ( var j in clUnit.idxUnitList )
      {
         var idxUnit = clUnit.idxUnitList[j] ;
         if ( idxUnit.getUpgradeType() == IDX_TYPE_CAN_UPGRADE )
         {
            upgradeIdx( clUnit._clName, idxUnit.getIdxDef() ) ;
         }
      }
   }

   for ( var i in UPGRADE_MAINCLUNIT_LIST )
   {
      var clUnit = UPGRADE_MAINCLUNIT_LIST[i] ;
      for ( var j in clUnit.canUpgradeIdxDefList )
      {
         upgradeIdx( clUnit._clName, clUnit.canUpgradeIdxDefList[j] ) ;
      }
   }
}

function isCatalogUpgrade()
{
  /*
   * check catalog node has been upgrade or not
   */
   // loop every node
   for ( var j in CATALOG_NODE_LIST )
   {
      var arr = CATALOG_NODE_LIST[j].split( ":" ) ;
      var hostname = arr[0] ;
      var svcname = arr[1] ;
      var dataDb = new Sdb( hostname, svcname, USER ) ;

      // loop every system collection
      var rc1 = dataDb.list( SDB_LIST_COLLECTIONS ) ;
      while( rc1.next() )
      {
         var clFullName  = rc1.current().toObj().Name ;
         var csName      = clFullName.split( "." )[0] ;
         var clShortName = clFullName.split( "." )[1] ;
         var collection  = dataDb.getCS( csName ).getCL( clShortName ) ;

         // loop every index
         var rc2 = collection.listIndexes() ;
         while ( rc2.next() )
         {
            var def = rc2.current().toObj().IndexDef ;
            if ( def.UniqueID == undefined )
            {
               return false ;
            }

         }
      }
   }

   return true ;
}

function upgradeSysCollectionAtNode( nodeName )
{
   var arr = nodeName.split( ":" ) ;
   var hostname = arr[0] ;
   var svcname = arr[1] ;
   var dataDb = new Sdb( hostname, svcname, USER ) ;

   // loop every system collection
   var rc1 = dataDb.list( SDB_LIST_COLLECTIONS ) ;
   while( rc1.next() )
   {
      var clFullName  = rc1.current().toObj().Name ;

      // only upgrade system collection
      if ( clFullName.substr(0,3) != "SYS" )
      {
         continue ;
      }

      // get colleciton
      var csName      = clFullName.split( "." )[0] ;
      var clShortName = clFullName.split( "." )[1] ;
      var collection  = dataDb.getCS( csName ).getCL( clShortName ) ;

      // loop every index
      var rc2 = collection.listIndexes() ;
      while ( rc2.next() )
      {
         var def = rc2.current().toObj().IndexDef ;
         if ( def.UniqueID == undefined )
         {
            var attr = getIdxAttr( def ) ;
            try
            {
               if ( def.name == "$id" )
               {
                  collection.createIdIndex() ;
               }
               else
               {
                  collection.createIndex( def.name, def.key, attr ) ;
               }
            }
            catch( e )
            {
               if ( e != -247 )
               {
                  println( "Failed to upgrade index: nodeName[" +
                           nodeName + "] clName[" + clFullName +
                           "] indexName[" + def.name + "] rc: " + e ) ;
               }
            }
         }

      }
   }
   dataDb.close() ;
}

function upgradeSysCollection()
{
   // If catalog has been upgraded, means all groups's system collection
   // has been upgraded
   if ( isCatalogUpgrade() )
   {
      return ;
   }

   // upgrade data node's system collection's index
   var nodeListList = GROUP_NODES_MAP.getValues() ;
   for ( var i in nodeListList )
   {
      // loop every node
      var nodeList = nodeListList[i] ;
      for ( var j in nodeList )
      {
         var nodeName = nodeList[j] ;
         upgradeSysCollectionAtNode( nodeName ) ;
      }
   }

   // upgrade data node's system collection's index
   for ( var j in CATALOG_NODE_LIST )
   {
      var nodeName = CATALOG_NODE_LIST[j] ;
      upgradeSysCollectionAtNode( nodeName ) ;
   }
}

function upgradeIdx( clName, idxDef )
{
   var csName = clName.split( "." )[0] ;
   var clShortName = clName.split( "." )[1] ;
   var errCode = 0 ;

   var collection = db.getCS( csName ).getCL( clShortName ) ;

   try
   {
      if ( idxDef.name == "$id" )
      {
         collection.createIdIndex() ;
      }
      else if ( idxDef.name == "$shard" )
      {
         collection.enableSharding( { ShardingKey: idxDef.key } ) ;
      }
      else
      {
         collection.createIndex( idxDef.name, idxDef.key,
                                 getIdxAttr( idxDef ) ) ;
      }
   }
   catch( e )
   {
      if ( e != -247 )
      {
         errCode = e ;
      }
   }

   UPGRADE_RESULT_FMT.push( clName, idxDef.name, errCode ) ;
}

function isEqualIdx( def1, def2 ) 
{
   delete def1._id ;
   delete def2._id ;
   delete def1.UniqueID ;
   delete def2.UniqueID ;
   delete def1.CreateTime ;  // v5.0.3 has this field
   delete def2.CreateTime ;  // v5.0.3 has this field
   delete def1.RebuildTime ; // v5.0.3 has this field   
   delete def2.RebuildTime ; // v5.0.3 has this field   
   delete def1.Standalone ;
   delete def2.Standalone ;
   return isEqual( def1, def2 )
}

function isConflictIdx( def1, def2 )
{ 
   if ( isEqual( def1.name, def2.name ) )
   {
      return true ;
   }
   return compareIndexDef( def1, def2 ) ;
}

function isObjInclude( obj, subObj )
{
   for ( var i in subObj )
   {
      if ( i in obj )
      {
         // include
      }
      else
      {
         return false ;
      }
   }
   return true ;
}

function getIdxAttr( idxDef )
{
   var attr = {} ;
   if ( idxDef.unique )
   {
      attr.unique = true ;
   }
   if ( idxDef.enforced )
   {
      attr.enforced = true ;
   }
   if ( idxDef.NotNull )
   {
      attr.NotNull = true ;
   }
   if ( idxDef.NotArray )
   {
      attr.NotArray = true ;
   }
   if ( idxDef.Global )
   {
      attr.Global = true ;
   }

   return attr ;
}

function getIdxAttrDesc( idxDef )
{
   var attr = "" ;
   if ( idxDef.unique )
   {
      attr += "|Unique" ;
   }
   if ( idxDef.enforced )
   {
      attr += "|Enforced" ;
   }
   if ( idxDef.NotNull )
   {
      attr += "|NotNull" ;
   }
   if ( idxDef.NotArray )
   {
      attr += "|NotArray" ;
   }
   if ( idxDef.Global )
   {
      attr += "|Global" ;
   }

   if ( attr.length > 0 )
   {
      return attr.substr( 1 ) ;
   }
   else
   {
      return "-" ;
   }
}

function NoNeedUpgradeFormator()
{
   this._id = 0 ;
   this._clNameList = [] ;
   this._idxNameList = [] ;
   this._idxTypeList = [] ;
   this._maxCLLen = 0 ;
   this._maxIdxLen = 0 ;
   this._maxTypeLen = 0 ;
   this.push = function( collection, idxName, idxType )
   {
      this._id++ ;
      this._clNameList.push( collection ) ;
      this._idxNameList.push( idxName ) ;
      this._idxTypeList.push( idxType ) ;
      this._maxCLLen   = Math.max( collection.length, this._maxCLLen ) ;
      this._maxIdxLen  = Math.max( idxName.length,    this._maxIdxLen ) ;
      this._maxTypeLen = Math.max( idxType.length,    this._maxTypeLen ) ;
      return this._id ;
   }
   this.count = function()
   {
      return this._id ;
   }
   this.print = function()
   {
      // get pad length
      var idLen = Math.max( this._id.toString().length, FIELD_ID.length + 1 ) ;
      var clLen = Math.max( this._maxCLLen, FIELD_COLLECTION.length + 5 ) ;
      clLen = Math.min( clLen, 20 ) ;
      var idxLen = Math.max( this._maxIdxLen, FIELD_INDEXNAME.length + 5 ) ;
      idxLen = Math.min( idxLen, 20 ) ;
      var typeLen = Math.max( this._maxTypeLen, FIELD_INDEXTYPE.length + 3 ) ;
      typeLen = Math.min( typeLen, 20 ) ;
      // format
      var str = "===================== Check Result ( No Need to Upgrade ) ======================\n" +
                pad( FIELD_ID,         idLen )   + " " +
                pad( FIELD_COLLECTION, clLen )   + " " +
                pad( FIELD_INDEXNAME,  idxLen )  + " : " +
                pad( FIELD_INDEXTYPE,  typeLen ) + "\n" ;
      for ( var i = 0 ; i < this._id ; i++ )
      {
         str += pad( i + 1,                idLen )   + " " +
                pad( this._clNameList[i],  clLen )   + " " +
                pad( this._idxNameList[i], idxLen )  + " : " +
                pad( this._idxTypeList[i], typeLen ) + "\n" ;
      }
      println( str ) ;
   }
}

function CanUpgradeFormator()
{
   this._id = 0 ;
   this._clNameList = [] ;
   this._idxNameList = [] ;
   this._idxKeyList = [] ;
   this._idxAttrList = [] ;
   this._idxTypeList = [] ;
   this._maxCLLen = 0 ;
   this._maxNameLen = 0 ;
   this._maxKeyLen = 0 ;
   this._maxAttrLen = 0 ;
   this._maxTypeLen = 0 ;
   this.push = function( collection, idxName, keyStr, attrStr, idxType )
   {
      this._id++ ;
      this._clNameList.push( collection ) ;
      this._idxNameList.push( idxName ) ;
      this._idxKeyList.push( keyStr ) ;
      this._idxAttrList.push( attrStr ) ;
      this._idxTypeList.push( idxType ) ;
      this._maxCLLen   = Math.max( collection.length, this._maxCLLen ) ;
      this._maxNameLen = Math.max( idxName.length,    this._maxNameLen ) ;
      this._maxKeyLen  = Math.max( keyStr.length,     this._maxKeyLen ) ;
      this._maxAttrLen = Math.max( attrStr.length,    this._maxAttrLen ) ;
      this._maxTypeLen = Math.max( idxType.length,    this._maxTypeLen ) ;
      return this._id ;
   }
   this.count = function()
   {
      return this._id ;
   }
   this.print = function()
   {
      // get pad length
      var idLen = Math.max( this._id.toString().length, FIELD_ID.length + 1 ) ;
      var clLen = Math.max( this._maxCLLen, FIELD_COLLECTION.length + 5 ) ;
      clLen = Math.min( clLen, 20 ) ;
      var nameLen = Math.max( this._maxNameLen, FIELD_INDEXNAME.length + 5 ) ;
      nameLen = Math.min( nameLen, 20 ) ;
      var keyLen = Math.max( this._maxKeyLen, FIELD_INDEXKEY.length + 5 ) ;
      keyLen = Math.min( keyLen, 20 ) ;
      var attrLen = Math.max( this._maxAttrLen, FIELD_INDEXATTR.length + 3 ) ;
      attrLen = Math.min( attrLen, 24 ) ;
      var typeLen = Math.max( this._maxTypeLen, FIELD_INDEXTYPE.length + 3 ) ;
      typeLen = Math.min( typeLen, 20 ) ;
      // format
      var str = "===================== Check Result ( Can be Upgraded ) =========================\n" +
                pad( FIELD_ID,         idLen )   + " " +
                pad( FIELD_COLLECTION, clLen )   + " " +
                pad( FIELD_INDEXNAME,  nameLen ) + " : " +
                pad( FIELD_INDEXKEY,   keyLen )  + " " +
                pad( FIELD_INDEXATTR,  attrLen ) + " " +
                pad( FIELD_INDEXTYPE,  typeLen ) + "\n" ;
      for ( var i = 0 ; i < this._id ; i++ )
      {
         str += pad( i + 1,                idLen )   + " " +
                pad( this._clNameList[i],  clLen )   + " " +
                pad( this._idxNameList[i], nameLen ) + " : " +
                pad( this._idxKeyList[i],  keyLen )  + " " +
                pad( this._idxAttrList[i], attrLen ) + " " +
                pad( this._idxTypeList[i], typeLen ) + "\n" ;
      }
      println( str ) ;
   }
}

function CannotUpgradeFormator()
{
   this._id = 0 ;
   this._clNameList = [] ;
   this._idxNameList = [] ;
   this._idxKeyList = [] ;
   this._idxAttrList = [] ;
   this._reasonList = [] ;
   this._maxCLLen = 0 ;
   this._maxNameLen = 0 ;
   this._maxKeyLen = 0 ;
   this._maxAttrLen = 0 ;
   this._maxReasonLen = 0 ;
   this.push = function( collection, idxName, keyStr, attrStr, reason )
   {
      this._id++ ;
      this._clNameList.push( collection ) ;
      this._idxNameList.push( idxName ) ;
      this._idxKeyList.push( keyStr ) ;
      this._idxAttrList.push( attrStr ) ;
      this._reasonList.push( reason ) ;
      this._maxCLLen     = Math.max( collection.length, this._maxCLLen ) ;
      this._maxNameLen   = Math.max( idxName.length,    this._maxNameLen ) ;
      this._maxKeyLen    = Math.max( keyStr.length,     this._maxKeyLen ) ;
      this._maxAttrLen   = Math.max( attrStr.length,    this._maxAttrLen ) ;
      this._maxReasonLen = Math.max( reason.length,     this._maxReasonLen ) ;
      return this._id ;
   }
   this.count = function()
   {
      return this._id ;
   }
   this.print = function()
   {
      // get pad length
      var idLen = Math.max( this._id.toString().length, FIELD_ID.length + 1 ) ;
      var clLen = Math.max( this._maxCLLen, FIELD_COLLECTION.length + 5 ) ;
      clLen = Math.min( clLen, 20 ) ;
      var nameLen = Math.max( this._maxNameLen, FIELD_INDEXNAME.length + 5 ) ;
      nameLen = Math.min( nameLen, 20 ) ;
      var keyLen = Math.max( this._maxKeyLen, FIELD_INDEXKEY.length + 5 ) ;
      keyLen = Math.min( keyLen, 20 ) ;
      var attrLen = Math.max( this._maxAttrLen, FIELD_INDEXATTR.length + 3 ) ;
      attrLen = Math.min( attrLen, 24 ) ;
      var reaLen = Math.max( this._maxReasonLen, FIELD_REASON.length + 5 ) ;
      reaLen = Math.min( reaLen, 20 ) ;
      // format
      var str = "===================== Check Result ( Cannot be Upgraded ) ======================\n" +
                pad( FIELD_ID,         idLen )   + " " +
                pad( FIELD_COLLECTION, clLen )   + " " +
                pad( FIELD_INDEXNAME,  nameLen ) + " : " +
                pad( FIELD_INDEXKEY,   keyLen )  + " " +
                pad( FIELD_INDEXATTR,  attrLen ) + " " +
                pad( FIELD_REASON,     reaLen ) + "\n" ;
      for ( var i = 0 ; i < this._id ; i++ )
      {
         str += pad( i + 1,                idLen )   + " " +
                pad( this._clNameList[i],  clLen )   + " " +
                pad( this._idxNameList[i], nameLen ) + " : " +
                pad( this._idxKeyList[i],  keyLen )  + " " +
                pad( this._idxAttrList[i], attrLen ) + " " +
                pad( this._reasonList[i],  reaLen )  + "\n" ;
      }
      println( str ) ;
   }
}

function DetailFormator()
{
   this._str = "" ;
   this.pushMissing = function ( id, idxUnit )
   {
      // get max length
      var maxGroupLen = 0 ;
      var maxNodeLen = 0 ;
      for ( var i in idxUnit.groupUnitList )
      {
         var gUnit = idxUnit.groupUnitList[i] ;
         maxGroupLen = Math.max( gUnit.groupName.length, maxGroupLen ) ;
         for ( var j in gUnit.nodeNameList )
         {
            maxNodeLen = Math.max( gUnit.nodeNameList[j].length, maxNodeLen ) ;
         }
      }
      // get pad length
      var groupLen = Math.max( maxGroupLen, FIELD_GROUPNAME.length + 1 ) ;
      var nodeLen  = Math.max( maxNodeLen,  FIELD_NODENAME.length + 12 ) ;
      var missLen  = FIELD_MISSING.length + 1 ;
      // format
      this._str += "  " + "---------- Index ( ID: " + id + " ) Missing -----------\n" +
                   "  " + pad( FIELD_GROUPNAME, groupLen ) + " " +
                          pad( FIELD_NODENAME,  nodeLen )  + " " +
                          pad( FIELD_MISSING,   missLen )  + "\n" ;
      for ( var i in idxUnit.groupUnitList )
      {
         var gUnit = idxUnit.groupUnitList[i] ;
         for ( var j in gUnit.nodeNameList )
         {
            var missStr = gUnit.hasIndexList[j] ? "N" : "Y" ;
            this._str += "  " + pad( gUnit.groupName,       groupLen ) + " " +
                                pad( gUnit.nodeNameList[j], nodeLen )  + " " +
                                pad( missStr,               missLen )  + "\n" ;
         }
      }
      this._str += "\n" ;
   }
   this.pushConflict = function ( id, idxUnitList )
   {
      // get max length
      var maxNameLen = 0 ;
      var maxKeyLen = 0 ;
      var maxAttrLen = 0 ;
      var maxGroupLen = 0 ;
      for ( var x in idxUnitList )
      {
         var idxUnit = idxUnitList[x] ;
         maxNameLen = Math.max( idxUnit.getIdxName().length, maxNameLen ) ;
         maxKeyLen  = Math.max( JSON.stringify( idxUnit.getIdxKey() ).length,
                                maxKeyLen ) ;
         maxAttrLen = Math.max( getIdxAttrDesc( idxUnit.getIdxDef() ).length,
                                maxAttrLen ) ;
         for ( var i in idxUnit.groupUnitList )
         {
            var gUnit = idxUnit.groupUnitList[i] ;
            for ( var j in gUnit.hasIndexList )
            {
               if ( gUnit.hasIndexList[j] )
               {
                  maxGroupLen = Math.max( gUnit.groupName.length, maxGroupLen ) ;
                  break ;
               }
            }
         }
      }
      // get pad length
      var nameLen = Math.max( maxNameLen, FIELD_INDEXNAME.length + 1 ) ;
      nameLen = Math.min( nameLen, 20 ) ;
      var keyLen = Math.max( maxKeyLen, FIELD_INDEXKEY.length + 2 ) ;
      keyLen = Math.min( keyLen, 20 ) ;
      var attrLen = Math.max( maxAttrLen, FIELD_INDEXATTR.length + 1 ) ;
      attrLen = Math.min( attrLen, 24 ) ;
      var groupLen = Math.max( maxGroupLen, FIELD_GROUPNAME.length + 1 ) ;
      groupLen = Math.min( groupLen, 15 ) ;
      // format
      this._str += "  " + "---------- Index ( ID: " + id + " ) Conflict -----------\n" +
                   "  " + pad( FIELD_INDEXNAME, nameLen )  + " " +
                          pad( FIELD_INDEXKEY,  keyLen )   + " " +
                          pad( FIELD_INDEXATTR, attrLen )  + " " +
                          pad( FIELD_GROUPNAME, groupLen ) + "  " +
                          FIELD_NODENAME                   + "\n" ;
      for ( var x in idxUnitList )
      {
         var idxUnit = idxUnitList[x] ;
         this._str += "  " +
                      pad( idxUnit.getIdxName(), nameLen ) + " " +
                      pad( JSON.stringify( idxUnit.getIdxKey() ), keyLen ) + " " +
                      pad( getIdxAttrDesc( idxUnit.getIdxDef() ), attrLen ) + " " ;
         var lineNum = 0 ;
         for ( var i in idxUnit.groupUnitList )
         {
            // build NodeName
            var gUnit = idxUnit.groupUnitList[i] ;
            var nodeStr = "" ;
            for ( var j in gUnit.hasIndexList )
            {
               if ( gUnit.hasIndexList[j] )
               {
                  if ( nodeStr.length != 0 )
                  {
                     nodeStr += "," ;
                  }
                  nodeStr += gUnit.nodeNameList[j] ;
               }
            }
            if ( nodeStr.length != 0 )
            {
               lineNum++ ;
               if ( lineNum != 1 )
               {
                  // +5: before groupname has 5 space
                  this._str += pad( "", nameLen + keyLen + attrLen + 5 ) ;
               }
               this._str += pad( gUnit.groupName, groupLen ) + "  " +
                            nodeStr                          + "\n" ;
            }
         }
      }
      this._str += "\n" ;
   }
   this.pushLocalCL = function( id, collection, nodeNameList )
   {
      this._printCLNode( id, collection, nodeNameList, FIELD_LOCAL_CL ) ;
   }
   this.pushAutoIdxId = function( id, collection, idxUnit )
   {
      var nodeNameList = [] ;
      for ( var i in idxUnit.groupUnitList )
      {
         var gUnit = idxUnit.groupUnitList[i] ;
         for ( var j in gUnit.nodeNameList )
         {
            if ( gUnit.hasIndexList[j] )
            {
               nodeNameList.push( gUnit.nodeNameList[j] ) ;
            }
         }
      }
      this._printCLNode( id, collection, nodeNameList, FIELD_AUTOINDEXID ) ;
   }
   this.pushEnsureShard = function( id, collection, idxUnit )
   {
      var nodeNameList = [] ;
      for ( var i in idxUnit.groupUnitList )
      {
         var gUnit = idxUnit.groupUnitList[i] ;
         for ( var j in gUnit.nodeNameList )
         {
            if ( gUnit.hasIndexList[j] )
            {
               nodeNameList.push( gUnit.nodeNameList[j] ) ;
            }
         }
      }
      this._printCLNode( id, collection, nodeNameList, FIELD_ENSURESHARD ) ;
   }
   this._printCLNode = function( id, collection, nodeNameList, title )
   {
      // get max length
      var maxNodeLen = 0 ;
      for ( var i in nodeNameList )
      {
         maxNodeLen = Math.max( nodeNameList[i].length, maxNodeLen ) ;
      }
      // get pad length
      var clLen = Math.max( collection.length, FIELD_COLLECTION.length + 1 ) ;
      var nodeLen = Math.max( maxNodeLen, FIELD_NODENAME.length + 12 ) ;
      // format
      this._str += "  " + "---------- Index ( ID: " + id + " ) " + title + " -----------\n" +
                   "  " + pad( FIELD_COLLECTION, clLen )   + " " +
                          pad( FIELD_NODENAME,   nodeLen ) + "\n" ;
      for ( var i in nodeNameList )
      {
         this._str += "  " + pad( collection,      clLen )   + " " +
                             pad( nodeNameList[i], nodeLen ) + "\n" ;
      }
      this._str += "\n" ;
   }

   this.print = function()
   {
      println( this._str ) ;
   }
}

function UpgradeResultFormator()
{
   this._id = 0 ;
   this._clNameList = [] ;
   this._idxNameList = [] ;
   this._resultCodeList = [] ;
   this._maxCLLen = 0 ;
   this._maxIdxLen = 0 ;
   this._succCnt = 0 ;
   this._failCnt = 0 ;
   this.push = function( collection, idxName, resultCode )
   {
      this._id++ ;
      this._clNameList.push( collection ) ;
      this._idxNameList.push( idxName ) ;
      this._resultCodeList.push( resultCode ) ;
      this._maxCLLen  = Math.max( collection.length, this._maxCLLen ) ;
      this._maxIdxLen = Math.max( idxName.length,    this._maxIdxLen ) ;
      if ( 0 == resultCode )
      {
         this._succCnt++ ;
      }
      else
      {
         this._failCnt++ ;
      }
      return this._id ;
   }
   this.count = function()
   {
      return this._id ;
   }
   this.succCnt = function()
   {
      return this._succCnt ;
   }
   this.failCnt = function()
   {
      return this._failCnt ;
   }
   this.print = function()
   {
      // get pad length
      var idLen = Math.max( this._id.toString().length, FIELD_ID.length + 1 ) ;
      var clLen = Math.max( this._maxCLLen, FIELD_COLLECTION.length + 5 ) ;
      clLen = Math.min( clLen, 20 ) ;
      var idxLen = Math.max( this._maxIdxLen, FIELD_INDEXNAME.length + 5 ) ;
      idxLen = Math.min( idxLen, 20 ) ;
      var rstLen = FIELD_RESULT.length + 3 ;
      var codeLen = FIELD_RESULTCODE.length + 2 ;
      // format
      var str = "===================== Upgrade Index ============================================\n" +
                pad( FIELD_ID,         idLen )   + " " +
                pad( FIELD_COLLECTION, clLen )   + " " +
                pad( FIELD_INDEXNAME,  idxLen )  + " : " +
                pad( FIELD_RESULT,     rstLen )  + " " +
                pad( FIELD_RESULTCODE, codeLen ) + "\n" ;
      for ( var i = 0 ; i < this._id ; i++ )
      {
         var isSucc = ( 0 == this._resultCodeList[i] ) ;
         str += pad( i + 1,                                 idLen )   + " " +
                pad( this._clNameList[i],                   clLen )   + " " +
                pad( this._idxNameList[i],                  idxLen )  + " : " +
                pad( isSucc ? "Succeed" : "Fail",           rstLen )  + " " +
                pad( isSucc ? "" : this._resultCodeList[i], codeLen ) + "\n" ;
      }
      println( str ) ;
   }
}

function SuggestionFormator()
{
   this.print = function ()
   {
      println(
"For indexes that cannot be upgraded, you need to intervene.\n" +
"  For missing index on data nodes, you can choose one of the following\n" +
"  options:\n" +
"    Option 1\n" +
"      Description: Create missing indexes to become consistent index.\n" +
"      Operation:   Connect to coord node to create index.\n" +
"      Influence:   It may takes a long time to create index.\n" +
"    Option 2\n" +
"      Description: Make existing indexes become standalone index.\n" +
"      Operation:   Specify the data node which exists index to create\n" +
"                   standalone index, and UniqueID will generate for it.\n" +
"      Influence:   Generate UniqueID only at data node.\n" +
"\n" +
"  For conflicting index on data nodes, you can choose one of the\n" +
"  following options:\n" +
"    Option 1\n" +
"      Description: Drop conflicting indexes to become consistent index.\n" +
"      Operation:   Connect to data node with conflicting index to drop\n" +
"                   index, then connect to coord node to create index.\n" +
"      Influence:   It may takes a long time to create index.\n" +
"    Option 2\n" +
"      Description: Make existing indexes become standalone index.\n" +
"      Operation:   Specify the data node which exists index to create\n" +
"                   standalone index, and UniqueID will generate for it.\n" +
"      Influence:   Generate UniqueID only at data node.\n" +
"\n" +
"  For local collection's indexes, you can do nothing.\n"
) ;
   }
}

function TotalCntFormator()
{
   this._noNeedUpgrade = 0 ;
   this._canUpgrade = 0 ;
   this._cannnotUpgrade = 0 ;
   this._doUpgrade = false ;
   this._succUpgrade = 0 ;
   this._failUpgrade = 0 ;

   this.set = function ( noNeedCnt, canCnt, cannotCnt )
   {
      this._noNeedUpgrade = noNeedCnt ;
      this._canUpgrade = canCnt ;
      this._cannnotUpgrade = cannotCnt ;
   }
   this.setUpgradeInfo = function( succCnt, failCnt )
   {
      this._doUpgrade = true ;
      this._succUpgrade = succCnt ;
      this._failUpgrade = failCnt ;
   }
   this.print = function ()
   {
      var content = "++++++++++++++++++++++++++\n" +
                    "No need to upgrade : " + this._noNeedUpgrade  + "\n" +
                    "Cannot be upgraded : " + this._cannnotUpgrade + "\n" +
                    "   Can be upgraded : " + this._canUpgrade     + "\n" ;
      if ( this._doUpgrade )
      {
         content += "Succeed to Upgrade : " + this._succUpgrade + "\n" +
                    " Failed to Upgrade : " + this._failUpgrade + "\n" ;
      }
      content += "++++++++++++++++++++++++++\n" ;
      println( content ) ;
   }
}

function pad( s, len )
{
   var expLen = 0 ;
   if ( s.length > len )
   {
      expLen = s.length ;
   }
   else
   {
      expLen = len ;
   }
   var buf = "                                                           " + s ;
   return buf.substr( buf.length - expLen ) ;
}

// string, array
function GroupUnit( groupName, nodeNameList )
{
   this.groupName = groupName ;
   this.nodeNameList = nodeNameList.concat() ;
   // Correspond to nodeNameList one by one.
   // Indicate whether there has the index on the node.
   this.hasIndexList = new Array( nodeNameList.length ) ;

   for( var i=0 ; i < this.hasIndexList.length ; i++ )
   {
      this.hasIndexList[i] = 0 ;
   }

   this.setNodeHasIdx = function( nodeName )
   {
      var pos = this.nodeNameList.indexOf( nodeName ) ;
      if ( pos != -1 )
      {
         this.hasIndexList[ pos ] = 1 ;
      }
   }
   this.is = function( groupname )
   {
      return this.groupName == groupname ;
   }
   this.print = function()
   {
      println( "    groupName: " + this.groupName ) ;
      println( "    nodeNameList: " + JSON.stringify( this.nodeNameList ) ) ;
      println( "    hasIdxlist: " + JSON.stringify( this.hasIndexList ) ) ;
   }
}

function isLocalID( uniqueid )
{
   if ( 0 == ( uniqueid & 0x80000000 ) )
   {
      return false ;
   }
   else
   {
      return true ;
   }
}

// groupObjList format:
// [
//   { GroupName: "group1", NodeNameList: [ "hostname1:11810", ... ] },
//   ...
// ]
function IndexUnit( idxDef, isInCata, groupObjList )
{
   this._idxDef = {} ;  // exclude _id, UniqueID
   this._hasIdxInfoInCata = isInCata ;
   this.uniqueID = 0 ;
   this.groupUnitList = [] ;
   this._nodeCnt = 0 ;
   this._idxCnt = 0 ;
   this._consistentUID = true ; // all nodes have same unique id
   this._allLocalID = true ;    // all data nodes's id is local id
   this._conflictWithOtherIdx = false ;
   this._hasBeenSet = false ;

   if ( idxDef.UniqueID === undefined )
   {
      this._consistentUID = false ;
   }
   else
   {
      this.uniqueID = idxDef.UniqueID ;
   }
   delete idxDef._id ;
   delete idxDef.UniqueID ;
   this._idxDef = idxDef ;

   for ( var i in groupObjList )
   {
      var obj = groupObjList[i] ;
      var groupUnit = new GroupUnit( obj.GroupName, obj.NodeNameList ) ;
      this.groupUnitList.push( groupUnit ) ;
      this._nodeCnt += obj.NodeNameList.length ;
   }
   // nodeObjList format:
   // [
   //   { NodeName: "hostname1:11810", UniqueID: 123 },
   //   ...
   // ]
   this.set = function( groupName, nodeObjList )
   {
      for ( var i in this.groupUnitList )
      {
         var groupUnit = this.groupUnitList[i] ;
         if ( groupUnit.is( groupName ) )
         {
            for( var j in nodeObjList )
            {
               var obj = nodeObjList[j] ;
               groupUnit.setNodeHasIdx( obj.NodeName ) ;
               this._idxCnt++ ;
               if ( this._consistentUID && this.uniqueID != obj.UniqueID )
               {
                  this._consistentUID = false ;
               }
               if ( this._allLocalID && !isLocalID( obj.UniqueID ) )
               {
                  this._allLocalID = false ;
               }
            }
            break ;
         }
      }
      this._hasBeenSet = true ;
   }
   this.is = function( idxDef )
   {
      return isEqualIdx( this._idxDef, idxDef ) ;
   }
   this.hasBeenSet = function()
   {
      return this._hasBeenSet ;
   }
   this.getIdxKey = function()
   {
      return this._idxDef.key ;
   }
   this.getIdxName = function()
   {
      return this._idxDef.name ;
   }
   this.getIdxDef = function()
   {
      return this._idxDef ;
   }
   this.setConflict = function()
   {
      this._conflictWithOtherIdx = true ;
   }
   this.isConflict = function()
   {
      return this._conflictWithOtherIdx ;
   }
   this.getUpgradeType = function()
   {
      if ( this._conflictWithOtherIdx )
      {
         return IDX_TYPE_CONFLICT ;
      }
      if ( this._hasIdxInfoInCata &&
           this._idxCnt == this._nodeCnt && this._consistentUID )
      {
         return IDX_TYPE_CONSISTENT ;
      }
      if ( !this._hasIdxInfoInCata && this._allLocalID )
      {
         return IDX_TYPE_STANDALONE ;
      }
      if ( this._idxCnt == this._nodeCnt )
      {
         return IDX_TYPE_CAN_UPGRADE ;
      }
      else
      { 
         return IDX_TYPE_MISSING ;
      }
   }
   this.print = function()
   {
      println( "  idxInCata: "+this._hasIdxInfoInCata ) ;
      println( "  idxDef: "+JSON.stringify( this._idxDef ) ) ;
      println( "  conflict: "+this._conflictWithOtherIdx ) ;
      for ( var i in this.groupUnitList )
      {
         this.groupUnitList[i].print() ;
      }
   }
}

// return true:  index1 equal to index2
// return false: not equal
function compareIndexDef( def1, def2 )
{
   if ( isEqual( def1.key, def2.key ) )
   {
      if ( def1.unique )
      {
         return true ;
      }
      else if ( def2.unique )
      {
         // def1 is not unique, def2 is unique
         // when an non unique index already exists, unique index can be created
         return false ;
      }
      if ( def1.NotNull != def2.NotNull )
      {
         return false ;
      }
      if ( def1.NotArray != def2.NotArray )
      {
         return false ;
      }
      return true ;
   }
   else
   {
      return false ;
   }
}

// cataObjList format:
// [
//   { GroupName: "group1", NodeNameList: [ "hostname1:11810", ... ] },
//   ...
// ]
function ClusterCLUnit( clName, cataObjList, autoIndexId, ensureShardingIdx, 
                        mainclName )
{
   this._clName = clName ;
   this._cataObjList = cataObjList ;
   this.idxUnitList = [] ;
   this._defMap = new UtilMap( compareIndexDef ) ; // < index def, array of this.idxUnitList pos >
   this._nameMap = new UtilMap() ; // < index name, array of this.idxUnitList pos >
   this._canUpgradeCnt = 0 ;
   this._autoIndexId = autoIndexId ;
   this._ensureShardingIdxx = ensureShardingIdx ;
   this._mainclName = mainclName ;

   this.mainCLName = function()
   {
      return this._mainclName ;
   }
   this.addByCatalog = function( idxDef )
   {
      var idxUnit = new IndexUnit( idxDef, true, this._cataObjList ) ;
      this.idxUnitList.push( idxUnit ) ;
   }
   // nodeObjList format:
   // [
   //   { NodeName: "hostname1:11810", UniqueID: 123 },
   //   ...
   // ]
   this.setByData = function( idxDef, groupName, nodeObjList )
   {
      var found = false ;
      for ( var i in this.idxUnitList )
      {
         var idxUnit = this.idxUnitList[i] ;
         if ( idxUnit.is( idxDef ) )
         {
            // may be addByCatalog
            var firstSet = idxUnit.hasBeenSet() ? false : true ;
            idxUnit.set( groupName, nodeObjList ) ;
            if ( firstSet )
            {
               this._checkConflict( idxUnit, Number(i) ) ;
            }
            found = true ;
            break ;
         }
      }
      if ( !found )
      {
         var idxUnit = new IndexUnit( idxDef, false, this._cataObjList ) ;
         idxUnit.set( groupName, nodeObjList ) ;
         this.idxUnitList.push( idxUnit ) ;
         this._checkConflict( idxUnit, this.idxUnitList.length - 1 ) ;
      }
      //this.print() ;
   }
   this._checkConflict = function( idxUnit, pos )
   {
      // check it is conflict index by index key
      var posList = this._defMap.get( idxUnit.getIdxDef() ) ;
      if ( posList == null )
      {
         this._defMap.add( idxUnit.getIdxDef(), [ pos ] ) ;
      }
      else
      {
         if ( 1 == posList.length )
         {
            this.idxUnitList[posList[0]].setConflict() ;
         }
         idxUnit.setConflict() ;
         posList.push( pos ) ;
         this._defMap.set( idxUnit.getIdxDef(), posList ) ;
      }
      // check it is conflict index by index name
      posList = this._nameMap.get( idxUnit.getIdxName() ) ;
      if ( posList == null )
      {
         this._nameMap.add( idxUnit.getIdxName(), [ pos ] ) ;
      }
      else
      {
         if ( 1 == posList.length )
         {
            this.idxUnitList[posList[0]].setConflict() ;
         }
         idxUnit.setConflict() ;
         posList.push( pos ) ;
         this._nameMap.set( idxUnit.getIdxName(), posList ) ;
      }
   }
   this.canUpgradeCnt = function()
   {
      // call it after format()
      return this._canUpgradeCnt ;
   }
   this.format = function( noNeedFmtor, canFmtor, cannotFmtor, detailFmtor )
   {
      for ( var i in this.idxUnitList )
      {
         var idxUnit = this.idxUnitList[i] ;
         var type = idxUnit.getUpgradeType() ;
         // deal with AutoIndexId and EnsureShardingIndex first
         if ( !this._autoIndexId && "$id" == idxUnit.getIdxName() )
         {
            var id = cannotFmtor.push( this._clName,
                                       idxUnit.getIdxName(),
                                       JSON.stringify( idxUnit.getIdxKey() ),
                                       getIdxAttrDesc( idxUnit.getIdxDef() ),
                                       "AutoIndexId=false" ) ;
            detailFmtor.pushAutoIdxId( id, this._clName, idxUnit ) ;
            continue ;
         }
         else if ( !this._ensureShardingIdxx &&
                   "$shard" == idxUnit.getIdxName() )
         {
            var id = cannotFmtor.push( this._clName,
                                       idxUnit.getIdxName(),
                                       JSON.stringify( idxUnit.getIdxKey() ),
                                       getIdxAttrDesc( idxUnit.getIdxDef() ),
                                       "EnsureShardingIndex=false" ) ;
            detailFmtor.pushEnsureShard( id, this._clName, idxUnit ) ;
            continue ;
         }
         // push index info to formator
         switch( type )
         {
            case IDX_TYPE_CONSISTENT:
               noNeedFmtor.push( this._clName,
                                 idxUnit.getIdxName(),
                                 "Consistent" ) ;
               break ;
            case IDX_TYPE_STANDALONE:
               noNeedFmtor.push( this._clName,
                                 idxUnit.getIdxName(),
                                 "Standalone" ) ;
               break ;
            case IDX_TYPE_CAN_UPGRADE:
               canFmtor.push( this._clName,
                              idxUnit.getIdxName(),
                              JSON.stringify( idxUnit.getIdxKey() ),
                              getIdxAttrDesc( idxUnit.getIdxDef() ),
                              "Consistent" ) ;
               this._canUpgradeCnt++ ;
               break ;
            case IDX_TYPE_MISSING:
               // If index1 {name:'a1',key:{a:1}} is missing, but index2 
               // {name:'a2',key:{a:1},unique:true} exists, so index1 is conflict
               var conflictUnitList = [] ;
               conflictUnitList.push( this.idxUnitList[i] ) ;// push myself to print myself first
               if ( ! idxUnit.getIdxDef().unique )
               {
                  for ( var j in this.idxUnitList )
                  {
                     if ( j != i && this.idxUnitList[j].getIdxDef().unique &&
                          isEqual( idxUnit.getIdxKey(), 
                                   this.idxUnitList[j].getIdxKey() ) )
                     {
                        idxUnit.setConflict() ;
                        conflictUnitList.push( this.idxUnitList[j] ) ;
                     }
                  }
               }
               if ( conflictUnitList.length > 1 )
               {
                  var id = cannotFmtor.push( this._clName,
                                             idxUnit.getIdxName(),
                                             JSON.stringify( idxUnit.getIdxKey() ),
                                             getIdxAttrDesc( idxUnit.getIdxDef() ),
                                             "Conflict" ) ;
                  detailFmtor.pushConflict( id, conflictUnitList ) ;
               }
               else
               {
                  var id = cannotFmtor.push( this._clName,
                                             idxUnit.getIdxName(),
                                             JSON.stringify( idxUnit.getIdxKey() ),
                                             getIdxAttrDesc( idxUnit.getIdxDef() ),
                                             "Missing" ) ;
                  detailFmtor.pushMissing( id, idxUnit ) ;
               }
               break ;
            case IDX_TYPE_CONFLICT:
            {
               var id = cannotFmtor.push( this._clName,
                                          idxUnit.getIdxName(),
                                          JSON.stringify( idxUnit.getIdxKey() ),
                                          getIdxAttrDesc( idxUnit.getIdxDef() ),
                                          "Conflict" ) ;
               // remove duplicate pos
               var conflictPosList = this._defMap.get( idxUnit.getIdxDef() ) ;
               var conflictPosSet = new UtilSet() ;
               conflictPosSet.push( i ) ; // push myself to print myself first
               if ( conflictPosList != null )
               {
                  for ( var i in conflictPosList )
                  {
                     var pos = conflictPosList[i] ;
                     conflictPosSet.push( pos ) ;
                  }
               }
               var conflictPosList = this._nameMap.get( idxUnit.getIdxName() ) ;
               if ( conflictPosList != null )
               {
                  for ( var i in conflictPosList )
                  {
                     var pos = conflictPosList[i] ;
                     conflictPosSet.push( pos ) ;
                  }
               }
               // get conflict units
               var conflictUnitList = [] ;
               for ( var i in conflictPosSet.data )
               {
                  var pos = conflictPosSet.data[i] ;
                  conflictUnitList.push( this.idxUnitList[pos] ) ;
               }
               detailFmtor.pushConflict( id, conflictUnitList ) ;
               break ;
            }
         }
      }
   }
   this.print = function()
   {
      println( "clName: "+this._clName ) ;
      println( "cataObj: "+JSON.stringify( this._cataObjList ) ) ;
      for ( var i in this.idxUnitList )
      {
         this.idxUnitList[i].print() ;
      }
      print( "defMap: " ) ; this._defMap.print() ;
      print( "nameMap: " ) ; this._nameMap.print() ;
   }
}

function MainCLUnit( clName, shardingKey )
{
   this._clName = clName ;
   this._shardingKey = shardingKey ;
   this.catIdxDefList = [] ;
   this.canUpgradeIdxDefList = [] ;

   this.addByCatalog = function( idxDef )
   {
      this.catIdxDefList.push( idxDef ) ;
   }
   this.setBySubcl = function( idxDef )
   {
      if ( idxDef.unique && ! isObjInclude( idxDef.key, this._shardingKey ) )
      {
         // unique index should include sharding key
         return ;
      }
      var conflict = false ;
      for ( var i in this.catIdxDefList )
      {
         var catDef = this.catIdxDefList[i] ;
         if ( isConflictIdx( catDef, idxDef ) )
         {
            conflict = true ;
            break ;
         }
      }
      if ( !conflict )
      {
         this.canUpgradeIdxDefList.push( idxDef ) ;
      }
   }
   this.canUpgradeCnt = function()
   {
      return this.canUpgradeIdxDefList.length ;
   }
}

function LocalCLUnit( clName, nodeNameList )
{
   this._clName = clName ;
   this._nodeNameList = nodeNameList ;
   this.idxDefList = [] ;

   this.set = function( idxDef )
   {
      this.idxDefList.push( idxDef ) ;
   }
   this.format = function( cannotFmtor, detailFmtor )
   {
      for ( var i in this.idxDefList )
      {
         var def = this.idxDefList[i] ;
         var id = cannotFmtor.push( this._clName,
                                    def.name,
                                    JSON.stringify( def.key ),
                                    getIdxAttrDesc( def ),
                                    "Local CL" ) ;
         detailFmtor.pushLocalCL( id, this._clName, this._nodeNameList ) ;
      }
   }
   this.print = function()
   {
      println( "clName: "+this._clName ) ;
      println( "nodeNameList: "+this._nodeNameList ) ;
      println( "idxDefList: "+JSON.stringify( this.idxDefList ) ) ;
      println( "hasUIDList: "+this.hasUIDList ) ;
   }
}

function UtilSet()
{
   this.data = [] ;
   this.push = function( key )
   {
      for( var i = 0 ; i < this.data.length ; i++ )
      {
         if ( isEqual( key, this.data[i] ) )
         {
            return ;
         }
      }
      this.data.push( key ) ;
   }
   this.clear = function()
   {
      this.data = [] ;
   }
   this.size = function()
   {
      return this.data.length ;
   }
}

function isEqual( k1, k2 )
{
   if ( k1.constructor == Object || k2.constructor == Object )
   {
      return JSON.stringify( k1 ) == JSON.stringify( k2 ) ;
   }
   else
   {
      return k1 == k2 ;
   }
}

function UtilMap( compareFunc )
{
   this._data = [] ;
   this._compareFunc = isEqual ;
   if ( typeof( compareFunc ) != "undefined" )
   {
      this._compareFunc = compareFunc ;
   }

   this.add = function( key, value )
   {
      this._data.push( { Key: key, Value: value } ) ;
   }
   this.set = function( key, value )
   {
      var found = false ;
      for( var i = 0 ; i < this._data.length ; i++ )
      {
         if ( this._compareFunc( this._data[i].Key, key ) )
         {
            this._data[i] = { Key: key, Value: value } ;
            found = true ;
            break ;
         }
      }
      if ( !found )
      {
         this._data.push( { Key: key, Value: value } ) ;
      }
   }
   this.get = function( key )
   {
      for( var i = 0 ; i < this._data.length ; i++ )
      {
         if ( this._compareFunc( this._data[i].Key, key ) )
         {
            return this._data[i].Value ;
         }
      }
      return null ;
   }
   this.remove = function( key )
   {
      for( var i = 0 ; i < this._data.length ; i++ )
      {
         if ( this._compareFunc( this._data[i].Key, key ) )
         {
            this._data.splice( i, 1 ) ;
            return ;
         }
      }
   }
   this.clear = function()
   {
      this._data = [] ;
   }
   this.size = function()
   {
      return this._data.length ;
   }
   this.has = function( key )
   {
      return -1 != this.pos( key ) ;
   }
   this.getKeys = function( )
   {
      var arr = [] ;
      for( var i = 0 ; i < this._data.length ; i++ )
      {
         arr.push( this._data[i].Key ) ;
      }
      return arr ;
   }
   this.getValues = function( )
   {
      var arr = [] ;
      for( var i = 0 ; i < this._data.length ; i++ )
      {
         arr.push( this._data[i].Value ) ;
      }
      return arr ;
   }
   this.clone = function()
   {
      var newObj = new UtilMap() ;
      newObj._data = this._data.concat() ;
      return newObj ;
   }
   this.print = function()
   {
      // print: [group1:100,group2:0,group3:100,... ]
      println( "---Map: " ) ;
      for( var i in this._data )
      {
         var k = this._data[i].Key ;
         var v = this._data[i].Value ;
         if ( k.constructor == Object || k.constructor == Array )
         {
            print( JSON.stringify( k ) ) ;
         }
         else
         {
            print( k ) ;
         }
         print( " " ) ;
         if ( v.constructor == Object || v.constructor == Array )
         {
            print( JSON.stringify( v ) ) ;
         }
         else
         {
            print( v ) ;
         }
         println() ;
      }
      println( "---end Map" ) ;
   }
}


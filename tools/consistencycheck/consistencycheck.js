/****************************************************************
@decription:   Upgrade index

@input:        hostname:   String, eg: "localhost", required
               svcname:    Number, eg: 11810, required
               username:   String, required
               password:   String, default: ""
               cipherfile: String
               token:      String

@example:
    ../../bin/sdb -f consistencycheck.js -e 'var hostname="localhost";
                                             var svcname=11810;var action="xxx";'

@author:       Ting YU 2021-03-23
               FangJiaBin 2024-11-20
****************************************************************/

import("./config.js")

var HOSTNAME = "" ;
var SVCNAME = "" ;
var USERNAME = "" ;
var PASSWD = "" ;
var CIPHER_FILE = "" ;
var TOKEN = "" ;
var CHECK_INFO_FILE_PATH = "" ;

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

if ( typeof( action ) === "undefined" )
{
   throw new Error( "no parameter [action] specified" ) ;
}
else if( action.constructor !== String )
{
   throw new Error( "Invalid para[action], should be String" ) ;
}
if ( "clear" != action && "check" != action && "generate" != action )
{
   throw new Error( "Invalid para[action], should be 'clear', or 'check' or 'generate'" ) ;
}

if ( typeof( outputFilePath ) === "undefined" )
{
   throw new Error( "no parameter [outputFilePath] specified" ) ;
}
else if( outputFilePath.constructor !== String )
{
   throw new Error( "Invalid para[outputFilePath], should be String" ) ;
}
CHECK_INFO_FILE_PATH = outputFilePath ;

/*************** main entry *****************/

var USER = null ;
var step = 1 ;
var STEP_NUM = 1 ;
if ( "clear" == action )
{
   STEP_NUM = _CLEAR_STEP_ARR.length ;
}
else if ( "check" == action )
{
   STEP_NUM = _CHECK_STEP_ARR.length ;
}
else if ( "generate" == action )
{
   STEP_NUM = _GENERATE_STEP_ARR.length ;
}

var checkInfos = "" ;
var infoCount = 0 ;
var jsCodes = "" ;
var codeCount = 0 ;
var supportOnlyUpgradeMeta = false ;
var oldVersion = false ;

function clearTmpCollectionSpace()
{
   oldVersion = false ;

   try
   {
      var beginTime = Date.now() ;
      println( "(" + step + "/" + STEP_NUM + ")Begin to clear tmp collection space" ) ;
      db.dropCS( TMP_CS_UPGRADE_INDEX, { "SkipRecycleBin": true } ) ;
   }
   catch( e )
   {
      if ( e != -34 && e != -6 )
      {
         println( "Failed to drop upgrade index tmp tables, rc: " + e ) ;
         throw e ;
      }
      if ( -6 == e )
      {
         oldVersion = true ;
      }
   }

   if ( oldVersion )
   {
      try
      {
         db.dropCS( TMP_CS_UPGRADE_INDEX ) ;
      }
      catch( e )
      {
         if ( e != -34 )
         {
            println( "Failed to drop upgrade index tmp tables, rc: " + e ) ;
            throw e ;
         }
      }
   }

   println( "(" + step + "/" + STEP_NUM + ")End to clear tmp collection space, spent time: " +
            ( (Date.now()) - beginTime )/1000 + "s" ) ;
   step++ ;
}

function truncateCL( collection )
{
   try
   {
      if ( oldVersion )
      {
         collection.truncate() ;
      }
      else
      {
         collection.truncate( { "SkipRecycleBin": true } ) ;
      }
   }
   catch( e )
   {
      println( "Failed to truncate cl, rc: " + e ) ;
      throw e ;
   }
}

function clearResultFiles( onlyJsFile )
{
   try
   {
      var beginTime = Date.now() ;
      println( "(" + step + "/" + STEP_NUM + ")Begin to clear result files" ) ;

      if ( File.exist( CHECK_INFO_FILE_PATH ) && !onlyJsFile )
      {
         File.remove( CHECK_INFO_FILE_PATH ) ;
      }
      if ( File.exist( UPGRADE_INDEX_JS_FILE ) )
      {
         File.remove( UPGRADE_INDEX_JS_FILE ) ;
      }
      if ( File.exist( MISS_INDEX_JS_FILE ) )
      {
         File.remove( MISS_INDEX_JS_FILE ) ;
      }
      if ( File.exist( CONFLICT_INDEX_JS_FILE ) )
      {
         File.remove( CONFLICT_INDEX_JS_FILE ) ;
      }
      if ( File.exist( INVALID_ID_INDEX_JS_FILE ) )
      {
         File.remove( INVALID_ID_INDEX_JS_FILE ) ;
      }
      if ( File.exist( LOCAL_CL_JS_FILE ) )
      {
         File.remove( LOCAL_CL_JS_FILE ) ;
      }

      println( "(" + step + "/" + STEP_NUM + ")End to clear result files, spent time: " +
               ( (Date.now()) - beginTime )/1000 + "s" ) ;
      step++ ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to clear generate js files, error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

function clear()
{
   clearTmpCollectionSpace() ;
   clearResultFiles( false ) ;
}

function init()
{
   try
   {
      var beginTime = Date.now() ;
      println( "(" + step + "/" + STEP_NUM + ")Begin to init tmp collections and result file" ) ;

      // 1. 创建临时表
      var cs = db.createCS( TMP_CS_UPGRADE_INDEX ) ;
      var cl = null ;
      cs.createCL( TMP_CL_CACHE_INFO ) ;
      cs.createCL( TMP_CL_GROUP_NODE_INFO ) ;
      cl = cs.createCL( TMP_CL_CATA_CLUSTER_CL_INFO ) ;
      cl.createIndex( "clFullName_idx", { ClFullName: 1 }, true );
      cs.createCL( TMP_CL_CATA_MAIN_CL_INFO ) ;
      cs.createCL( TMP_CL_DATA_CLUSTER_CL_INFO ) ;
      cs.createCL( TMP_CL_LOCAL_CL_INFO ) ;
      cl = cs.createCL( TMP_CL_DATA_INDEX_INFO ) ;
      cl.createIndex( "clFullName_idx", { ClFullName: 1 } ) ;
      cs.createCL( TMP_CL_CATA_INDEX_INFO ) ;
      cs.createCL( TMP_CL_DATA_INDEX_CHECK_INFO ) ;
      cs.createCL( TMP_CL_DATA_INDEX_CHECK_INFO_TMP ) ;
      cs.createCL( TMP_CL_CANNOT_UPGRADE_INDEX_INFO ) ;
      cs.createCL( TMP_CL_MAIN_CL_INDEX_CHECK_INFO ) ;
      cs.createCL( TMP_CL_NO_NEED_UPGRADE_INDEX_INFO ) ;
      cs.createCL( TMP_CL_INVALID_CL_INFO ) ;

      println( "(" + step + "/" + STEP_NUM + ")End to init tmp collections and result file, spent time: " +
               ( (Date.now()) - beginTime )/1000 + "s" ) ;
      step++ ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

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

main() ;

function main()
{
   var beginTime = Date.now() ;

   if ( "clear" == action )
   {
      clear() ;
   }
   else if ( "check" == action )
   {
      clear() ;

      if ( !preCheck1() )
      {
         println( "We don't need to upgrade indexes" ) ;
         return ;
      }

      init() ;

      collect() ;

      check() ;

      writeReport() ;

      println( "Generate report: " + CHECK_INFO_FILE_PATH ) ;
   }
   else if ( "generate" == action )
   {
      if ( !preCheck2() )
      {
         println( "Error: We must first execute the check process" ) ;
         return ;
      }
      checkIfSupportOnlyUpgradeMeta() ;
      clearResultFiles( true ) ;
      File.mkdir( JS_DIR ) ;
      generateJsScripts() ;
   }

   println( "Execute data consistency check done, spent time: " + ( (Date.now()) - beginTime )/1000 + "s" ) ;

   return "" ;
}

function preCheck1()
{
   try
   {
      var beginTime = Date.now() ;
      var rc = null ;
      var sqlStr = "" ;

      println( "(" + step + "/" + STEP_NUM + ")Begin to prepare check index info" ) ;

      sqlStr = "select count(1) as Count from $SNAPSHOT_CATA" ;
      rc = db.exec( sqlStr ) ;
      if ( rc.next() && 0 == rc.current().toObj().Count )
      {
         println( "Cata No Collections" ) ;
         return 0 ;
      }

      sqlStr = "select count(1) as Count from $SNAPSHOT_CL" ;
      rc = db.exec( sqlStr ) ;
      if ( rc.next() && 0 == rc.current().toObj().Count )
      {
         println( "Data No Collections" ) ;
         return 0 ;
      }

      println( "(" + step + "/" + STEP_NUM + ")End to prepare check index infos done, spent time: " +
                ( (Date.now()) - beginTime )/1000 + "s" ) ;
      step++ ;

      return 1 ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to pre check, error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

function preCheck2()
{
   try
   {
      var beginTime = Date.now() ;
      println( "(" + step + "/" + STEP_NUM + ")Begin to prepare check index info" ) ;
      db.getCS( TMP_CS_UPGRADE_INDEX ) ;
      println( "(" + step + "/" + STEP_NUM + ")End to prepare check index infos done, spent time: " +
                ( (Date.now()) - beginTime )/1000 + "s" ) ;
      step++ ;

      return 1 ;
   }
   catch( e )
   {
      if( -34 == e )
      {
         return 0 ;
      }
      println( "Failed to pre check, error Stack:\n" + e.stack ) ;
      throw e ;
   }
}

function checkIfSupportOnlyUpgradeMeta()
{
   try
   {
      var beginTime = Date.now() ;
      println( "(" + step + "/" + STEP_NUM + ")Begin to check if support only upgrade index meta" ) ;
      var cl = db.getCS( TMP_CS_UPGRADE_INDEX ).getCL( TMP_CL_CACHE_INFO ) ;
      cl.createIndex( "tmpIdx", { "tmpField": 1 }, {}, { "OnlyUpgradeMeta": true } ) ;
      cl.dropIndex( "tmpIdx" ) ;
      println( "Support OnlyUpgradeMeta parameter" ) ;
      println( "(" + step + "/" + STEP_NUM + ")End to check if support only upgrade index meta: " +
                ( (Date.now()) - beginTime )/1000 + "s" ) ;
      step++ ;

      supportOnlyUpgradeMeta = true ;
   }
   catch( e )
   {
      if( -6 == e )
      {
         println( "Don't support OnlyUpgradeMeta parameter" ) ;
         supportOnlyUpgradeMeta = false ;
      }
      else
      {
         println( "Failed to check, error Stack:\n" + e.stack ) ;
         throw e ;
      }
   }
}

function collectNodesInfo()
{
   /*

   {
      "GroupName": "SYSCatalogGroup",
      "NodeList": [
         {
            "HostName": "dell-c79-1701-3081",
            "NodeName": "30000"
         }
      ],
      "NodeCount": 1
   }

   {
      "GroupName": "db1",
      "NodeList": [
         {
            "HostName": "dell-c79-1701-3081",
            "NodeName": "20000"
         },
         {
            "HostName": "dell-c79-1701-3081",
            "NodeName": "21000"
         },
         {
            "HostName": "dell-c79-1701-3081",
            "NodeName": "22000"
         }
      ],
      "NodeCount": 3
   }

   */

   try
   {
      var beginTime = Date.now() ;
      var sqlStr1 = "select Group,GroupName from $LIST_GROUP where GroupName<>'SYSCoord' split by Group" ;
      var sqlStr2 = "select T1.GroupName as GroupName,T1.Group.HostName as HostName,T1.Group.Service.0.Name as NodeName from ( " + sqlStr1 + " ) as T1" ;
      var sqlStr3 = "select T2.GroupName as GroupName,buildobj(T2.HostName,T2.NodeName) as NodeName from ( " + sqlStr2 + " ) as T2" ;
      var sqlStr4 = "select T3.GroupName,addtoset(T3.NodeName) as NodeList,count(T3.NodeName) as NodeCount from ( " + sqlStr3 + " ) as T3 group by T3.GroupName" ;
      var sqlStr5 = "insert into " + TMP_CL_FULL_GROUP_NODE_INFO + " " + sqlStr4 ;

      println( "(" + step + "/" + STEP_NUM + ")Begin to collect groups and nodes info" ) ;
      db.execUpdate( sqlStr5 ) ;
      println( "(" + step + "/" + STEP_NUM + ")End to collect groups and nodes info, spent time: " +
               ( Date.now() - beginTime )/1000 + "s" ) ;
      step++ ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to collect groups and nodes info, error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

function collectCataClusterClInfo()
{
   /*
   T2 cl

   {
      "GroupName": "db2",
      "EnsureShardingIndex": true,
      "Attribute": 1,
      "MainClName": "csName_0.maincl",
      "Name": "csName_0.subcl_0",
      "ShardingKey": {
         "a": 1
      }
   }

   tmp_cl_cata_cluster_cl_info cl

   {
      "ClFullName": "csName_0.subcl_0",
      "MainClName": "csName_0.maincl",
      "EnsureShardingIndex": true,
      "ShardingKey": {
         "a": 1
      },
      "Attribute": 1,
      "Groups": [
         {
            "GroupName": "db2",
            "NodeList": [
            {
               "HostName": "u16-fjb",
               "NodeName": "40000"
            },
            {
               "HostName": "u16-fjb",
               "NodeName": "41000"
            },
            {
               "HostName": "u16-fjb",
               "NodeName": "42000"
            }
            ]
         },
         ...
      ]
   }

   */

   try
   {
      var beginTime = Date.now() ;

      var t1SqlStr = "select CataInfo,EnsureShardingIndex,MainCLName,Name,ShardingKey,Attribute from $SNAPSHOT_CATA where IsMainCL is null and " + SQL_COMMON_STR + " split by CataInfo" ;
      var t2SqlStr = "select T1.CataInfo.GroupName as GroupName,T1.EnsureShardingIndex as EnsureShardingIndex,T1.Attribute as Attribute,T1.MainCLName as MainClName,T1.Name as Name,T1.ShardingKey as ShardingKey from ( " + t1SqlStr + " ) as T1" ;
      var t3SqlStr = "select T2.GroupName,T2.EnsureShardingIndex,T2.MainClName,T2.Name,T2.ShardingKey,T2.Attribute,T3.NodeList,T3.NodeCount from " + TMP_CL_FULL_GROUP_NODE_INFO ;
      var joinSqlStr = t3SqlStr + " as T3 inner join ( " + t2SqlStr + " ) as T2 on T3.GroupName = T2.GroupName /*+use_hash()*/" ;
      var t5SqlStr = "select T4.EnsureShardingIndex as EnsureShardingIndex,T4.MainClName as MainClName, T4.Name as Name,T4.ShardingKey,T4.Attribute,buildobj( T4.GroupName,T4.NodeList) as GroupNodes,T4.NodeCount from ( " + joinSqlStr + " ) as T4" ;
      var t6SqlStr = "select T5.Name as ClFullName,T5.MainClName as MainClName,T5.EnsureShardingIndex as EnsureShardingIndex, T5.ShardingKey as ShardingKey,T5.Attribute as Attribute,addtoset(T5.GroupNodes) as Groups,count(T5.GroupNodes) as GroupCount,sum(T5.NodeCount) as NodeCount from ( " + t5SqlStr + " ) as T5 group by T5.Name" ;

      var sqlStr = "insert into " + TMP_CL_FULL_CATA_CLUSTER_CL_INFO + " " + t6SqlStr ;

      println( "(" + step + "/" + STEP_NUM + ")Begin to collect cata cluster collections info" ) ;
      db.execUpdate( sqlStr ) ;
      println( "(" + step + "/" + STEP_NUM + ")End to collect cata cluster collections info, spent time: " +
               ( (Date.now()) - beginTime )/1000 + "s" ) ;
      step++ ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to collect cata cluster collections info, error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

function collectCataMainClInfo()
{
   /*
   tmp_cl_cata_main_cl_info

   {
      "Name": "csName_9.maincl",
      "ShardingKey": {
         "a": 1
      },
      "SubCLList": [
         "csName_9.subcl_9",
         "csName_9.subcl_0",
         "csName_9.subcl_1",
         "csName_9.subcl_2",
         "csName_9.subcl_3",
         "csName_9.subcl_4",
         "csName_9.subcl_5",
         "csName_9.subcl_6",
         "csName_9.subcl_7",
         "csName_9.subcl_8"
      ],
      "SubCLCount": 10
   }

   */

   try
   {
      var beginTime = Date.now() ;

      var sqlStr1 = "select count(1) as Count from $SNAPSHOT_CATA where IsMainCL=true" ;

      var sqlStr2 = "insert into " + TMP_CL_FULL_CATA_MAIN_CL_INFO + " select T2.Name as Name,T2.ShardingKey as ShardingKey,addtoset(T2.SubCLName) as SubCLList,count(T2.SubCLName) as SubCLCount from (select T1.CataInfo.SubCLName as SubCLName,T1.Name as Name,T1.ShardingKey as ShardingKey from (select CataInfo,Name,ShardingKey from $SNAPSHOT_CATA where IsMainCL=true split by CataInfo) as T1) as T2 group by T2.Name" ;

      println( "(" + step + "/" + STEP_NUM + ")Begin to collect cata main collections info" ) ;

      var rc = db.exec( sqlStr1 ) ;
      if ( rc.next() && 0 == rc.current().toObj().Count )
      {
         println( "(" + step + "/" + STEP_NUM + ")Cata No Main Collections" ) ;
      }
      else
      {
         db.execUpdate( sqlStr2 ) ;
      }

      println( "(" + step + "/" + STEP_NUM + ")End to collect cata main collections info, spent time: " +
               ( (Date.now()) - beginTime )/1000 + "s" ) ;
      step++ ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to collect cata main collections info, error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

function collectDataClusterClInfo()
{
   /*
   tmp_cl_data_cluster_cl_info

   {
      "ClFullName": "csName_0.subcl_0",
      "DataClFullName": "csName_0.subcl_0",
      "Groups": [
         {
            "GroupName": "db1",
            "NodeList": [
               "u16-fjb:20000",
               "u16-fjb:22000",
               "u16-fjb:21000"
            ]
         },
         ...
      ],
      "GroupCount": 3,
      "NodeCount": 9
   }

   */

   try
   {
      var beginTime = Date.now() ;
      var t1SqlStr = "select * from $SNAPSHOT_CL where " + SQL_COMMON_STR + " split by Details" ;
      var t2SqlStr = "select T1.Name as Name,T1.Details.NodeName as NodeName,T1.Details.GroupName as GroupName from ( " + t1SqlStr + " ) as T1" ;
      var t3SqlStr = "select T2.Name as Name,T2.GroupName as GroupName,addtoset(T2.NodeName) as NodeTmpList from ( " + t2SqlStr + " ) as T2 group by T2.Name,T2.GroupName" ;
      var t4SqlStr = "select T3.Name as Name,T3.GroupName as GroupName,T3.NodeTmpList from ( " + t3SqlStr + " ) as T3 split by T3.NodeTmpList" ;
      var t5SqlStr = "select T4.Name as Name,T4.GroupName as GroupName,addtoset(T4.NodeTmpList) as NodeList,count(T4.NodeTmpList) as NodeCount from ( " + t4SqlStr + " ) as T4 group by T4.Name,T4.GroupName" ;
      var t6SqlStr = "select T5.Name as Name,buildobj(T5.GroupName,T5.NodeList) as Group,T5.NodeCount as NodeCount from ( " + t5SqlStr + " ) as T5" ;
      var sqlStr = "insert into " + TMP_CL_FULL_DATA_CLUSTER_CL_INFO + " select T6.Name as ClFullName,T6.Name as DataClFullName,addtoset(T6.Group) as Groups,count(T6.Group) as GroupCount,sum(T6.NodeCount) as NodeCount from ( " + t6SqlStr + " ) as T6 group by T6.Name" ;

      println( "(" + step + "/" + STEP_NUM + ")Begin to collect data cluster collections info" ) ;
      db.execUpdate( sqlStr ) ;
      println( "(" + step + "/" + STEP_NUM + ")End to collect data cluster collections info, spent time: " +
               ( (Date.now()) - beginTime )/1000 + "s" ) ;
      step++ ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to collect data cluster collections info, error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

function collectLocalClInfo()
{
   /*
   注意：cata_cluster_cl_info 会多出本地集合信息

   tmp_cl_local_cl_info

   {
      "ClFullName": "cs40000.cl",
      "DataClFullName": "cs40000.cl",
      "Groups": [
         {
            "GroupName": "db2",
            "NodeList": [
            "dell-c79-1701-3081:40000"
            ]
         }
      ],
      "GroupCount": 1,
      "NodeCount": 1
   }

   */

   try
   {
      var beginTime = Date.now() ;
      var sqlStr1 = "select * from " + TMP_CL_FULL_DATA_CLUSTER_CL_INFO ;
      var batchRecords = [] ;
      var cataClInfoCl = db.getCS( TMP_CS_UPGRADE_INDEX ).getCL( TMP_CL_CATA_CLUSTER_CL_INFO ) ;

      println( "(" + step + "/" + STEP_NUM + ")Begin to collect local collections info" ) ;
      var rc = db.exec( sqlStr1 ) ;
      while ( rc.next() )
      {
         batchRecords.push( rc.current().toObj() ) ;
         if ( batchRecords.length > BATCH_NUM )
         {
            cataClInfoCl.insert( batchRecords, SDB_INSERT_CONTONDUP ) ;
            batchRecords = [] ;
         }
      }
      if ( batchRecords.length > 0 )
      {
         cataClInfoCl.insert( batchRecords, SDB_INSERT_CONTONDUP ) ;
         batchRecords = [] ;
      }

      println( "(" + step + "/" + STEP_NUM + ")Collect local collections info done" ) ;

      var sqlStr2 = "insert into " + TMP_CL_FULL_LOCAL_CL_INFO + " select * from " + TMP_CL_FULL_CATA_CLUSTER_CL_INFO + " where DataClFullName is not null" ;
      db.execUpdate( sqlStr2 ) ;

      var sqlStr3 = "delete from " + TMP_CL_FULL_CATA_CLUSTER_CL_INFO + " where DataClFullName is not null" ;
      db.execUpdate( sqlStr3 ) ;

      println( "(" + step + "/" + STEP_NUM + ")Save local collections info done" ) ;

      println( "(" + step + "/" + STEP_NUM + ")End to collect local collections info, spent time: " +
               ( (Date.now()) - beginTime )/1000 + "s" ) ;
      step++ ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to collect local collections info, error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

function collectCataIndexInfo()
{
   /*
      {
         "ClFullName": "csName_0.subcl_0",
         "IndexName": "bIdx",
         "IndexDataType": 4294967298
      }
   */
   try
   {
      var beginTime = Date.now() ;
      var sqlStr = "insert into " + TMP_CL_FULL_CATA_INDEX_INFO + " select Collection as ClFullName, Name as IndexName, CLUniqueID as IndexDataType from $LIST_INDEXES" ;
      println( "(" + step + "/" + STEP_NUM + ")Begin to collect cata indexs info" ) ;
      db.execUpdate( sqlStr ) ;
      println( "(" + step + "/" + STEP_NUM + ")End to collect cata indexs info, spent time: " +
               ( (Date.now()) - beginTime )/1000 + "s" ) ;
      step++ ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to collect cata indexs info, error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

function generateCollectDataIndexInfoPlans( plans, clCount )
{
   try
   {
      var beginTime = Date.now() ;
      var rc = null ;
      var everyPlanClCount = 0 ;
      var planNum = 0 ;
      var coordAddrs = [] ;
      var plan = null ;
      var remainderClCount = 0 ;

      if ( clCount <= COLLECT_DATA_INDEX_INFO_THREAD_NUM )
      {
         planNum = clCount ;
         everyPlanClCount = 1 ;
         remainderClCount = 0 ;
      }
      else
      {
         planNum = COLLECT_DATA_INDEX_INFO_THREAD_NUM ;
         everyPlanClCount = Math.floor( clCount/COLLECT_DATA_INDEX_INFO_THREAD_NUM ) ;
         remainderClCount = clCount - everyPlanClCount * planNum ;
      }

      println( "(" + step + "/" + STEP_NUM + ")Begin to generate collect data indexs info plans[ plan num: " +
               planNum + ", collections num: " + clCount + " ]" ) ;

      var sqlStr = "select T1.Group.HostName,T1.Group.Service.0.Name as NodeName from (select * from $LIST_GROUP where GroupName='SYSCoord' split by Group) as T1" ;
      rc = db.exec( sqlStr ) ;
      while( rc.next() )
      {
         /*
         eg: obj = { HostName: "192.168.17.50", NodeName: 11810 }
         */
         coordAddrs.push( rc.current().toObj() ) ;
      }

      var skip = 0 ;
      for ( var i = 1 ; i <= planNum ; i++ )
      {
         var planClCount = everyPlanClCount ;
         var randomIndex = Math.floor( Math.random()*1000 ) % coordAddrs.length ;

         if ( remainderClCount > 0 )
         {
            planClCount += 1 ;
            remainderClCount -= 1 ;
         }

         plan = { "Skip": skip, "Limit": planClCount, "CoordAddr": coordAddrs[randomIndex],
                  "UserName": USERNAME, "Password": PASSWD, "Token": TOKEN,
                  "CipherFile": CIPHER_FILE, "CollectionsNum": planClCount } ;
         println( "(" + step + "/" + STEP_NUM + ")Generate plan done and it will collect " +
                  planClCount + " collections data index infos, use coord[" +
                  JSON.stringify( coordAddrs[randomIndex] ) + "]" ) ;

         plans.push( plan ) ;
         skip += planClCount ;
      }

      if ( clCount != skip )
      {
         throw new Error( "Invalid plans[expect cl num: " +
                          clCount + ", actual cl num: " + skip + "]\n" ) ;
      }

      println( "(" + step + "/" + STEP_NUM + ")End to generate collect data indexs info plans, spent time: " +
               ( (Date.now()) - beginTime )/1000 + "s" ) ;
      step++ ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to generate collect data indexs info plans, error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

function waitPlansDone( plans, clCount )
{
   var beginTime = Date.now() ;
   var hasPidExist = true ;
   var printProgressCount = _PRINT_PROGRESS_COUNT ;
   var tmpNum = clCount/printProgressCount ;
   tmpNum = tmpNum > 1 ? tmpNum : 1 ;
   var aProgressCollectClNum = tmpNum ;

   try
   {
      println( "(" + step + "/" + STEP_NUM + ")Begin to wait plans done" ) ;
      var dataInfoCl = db.getCS( TMP_CS_UPGRADE_INDEX ).getCL( TMP_CL_DATA_INDEX_INFO ) ;
      var cmd = new Cmd() ;

      // 每完成四十分之一打印一个 #
      println( "Collecting\n0% ______________ 50% ______________ 100%" ) ;

      while ( hasPidExist )
      {
         var plansHasCollectClNum = 0 ;

         hasPidExist = false ;

         sleep( COLLECT_DATA_INDEX_INFO_SLEEP_TIME ) ;

         for ( i in plans )
         {
            var plan = plans[i] ;
            var pid = "" + plan.PID ;
            var collectionNum = plan.CollectionsNum ;
            var filePath = PROGRESS_TMP_FILEPATH_PREFIXX + pid ;
            var pidExist = System.isProcExist( { "value": pid, "type": "pid" } ) ;
            if ( !pidExist )
            {
               if ( !File.exist( filePath ) )
               {
                  throw new Error( "Failed to collect data index info[plan: " +
                                   JSON.stringify(plan) + "], file[" + filePath + "] does not exist \n" ) ;
               }

               var file = new File( filePath ) ;
               var content = file.read() ;
               if ( 0 == content )
               {
                  /*
                  println( "(" + step + "/" + STEP_NUM + ")Plan[PID:" + pid +
                           "] succeccfully, spent time: " +
                           ( (Date.now()) - beginTime )/1000 + "s" ) ;*/
                  plansHasCollectClNum += collectionNum ;
                  continue ;
               }
               else
               {
                  throw new Error( "Failed to collect data index info[plan: " +
                                   JSON.stringify(plan) + "], error msg: " +
                                   content + "\n" ) ;
               }
            }
            else
            {
               if ( File.exist( filePath ) )
               {
                  plansHasCollectClNum += parseInt( cmd.run( "wc -l " + filePath + " | awk '{print $1}'" ) ) ;
               }
               hasPidExist = true ;
            }
         }

         while( plansHasCollectClNum >= tmpNum && 0 != printProgressCount )
         {
            print( "#" ) ;
            tmpNum += aProgressCollectClNum ;
            printProgressCount-- ;
         }
      }

      for ( var i = 0 ; i < printProgressCount ; i++ )
      {
         print( "#" ) ;
      }
      print( "\n" ) ;

      println( "(" + step + "/" + STEP_NUM + ")End to wait plans done[data index infos count: " +
               dataInfoCl.count() + "], spent time: " + ( (Date.now()) - beginTime )/1000 + "s" ) ;
      step++ ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to wait plans done, error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

function startCollectDataIndexInfoPlans( plans )
{
   try
   {
      var commandStr = "" ;
      var cmd = new Cmd() ;
      var beginTime = Date.now() ;
      println( "(" + step + "/" + STEP_NUM + ")Begin to start plans to collect data indexs info" ) ;
      for( i in plans )
      {
         var pid = 0 ;
         var plan = plans[i] ;
         commandStr = sdbShellPath + " -f " + collectDataIndexInfoJsFile +
                      " -e \'var PLAN=" + JSON.stringify( plan ) + "\'" ;
         plan.Command = commandStr ;
         //println( commandStr ) ;
         //println( cmd.run( commandStr ) ) ;
         pid = cmd.start( commandStr, "", 100, 0 ) ;
         //println( "PID: " + pid ) ;
         //println( "Last out: " + cmd.getLastOut() ) ;
         //println( "Last ret: " + cmd.getLastRet() ) ;
         println( "(" + step + "/" + STEP_NUM + ")Start plan[PID:" + pid + "] to collect data indexs info" ) ;
         plan.PID = pid ;
      }
      println( "(" + step + "/" + STEP_NUM + ")End to start plans to collect data indexs info, spent time: " +
               ( (Date.now()) - beginTime )/1000 + "s" ) ;
      step++ ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to start collect data indexs info plans, error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

function clearTmpFiles( plans )
{
   try
   {
      var beginTime = Date.now() ;
      println( "(" + step + "/" + STEP_NUM + ")Begin to clear tmp files" ) ;
      for( i in plans )
      {
         var filePath = PROGRESS_TMP_FILEPATH_PREFIXX + plans[i].PID ;
         if ( File.exist( filePath ) )
         {
            File.remove( filePath ) ;
         }
      }
      println( "(" + step + "/" + STEP_NUM + ")End to clear tmp files, spent time: " +
               ( (Date.now()) - beginTime )/1000 + "s" ) ;
      step++ ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to clear tmp files, error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

function collectDataIndexInfoConcurrent()
{
   var plans = [] ;
   var cl = null ;
   var clCount = 0 ;

   try
   {
      cl = db.getCS( TMP_CS_UPGRADE_INDEX ).getCL( TMP_CL_CATA_CLUSTER_CL_INFO ) ;
      clCount = cl.count( { "DataClFullName": { "$isnull": 1 } } ) ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to get cata cluster collections count, error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }

   generateCollectDataIndexInfoPlans( plans, clCount )

   startCollectDataIndexInfoPlans( plans ) ;

   waitPlansDone( plans, clCount ) ;

   clearTmpFiles( plans ) ;
}

function collectInvalidClInfos()
{
   try
   {
      var beginTime = Date.now() ;

      println( "(" + step + "/" + STEP_NUM + ")Begin to collect invalid collection inofs" ) ;

      var tmpCS = db.getCS( TMP_CS_UPGRADE_INDEX ) ;
      var invalidClInfoCl = tmpCS.getCL( TMP_CL_INVALID_CL_INFO ) ;
      var sqlStr1 = "select * from " + TMP_CL_FULL_LOCAL_CL_INFO ;
      var rc = db.exec( sqlStr1 ) ;
      while ( rc.next() )
      {
         var obj = rc.current().toObj() ;
         var clFullName = obj.ClFullName ;
         var csName = clFullName.split( "." )[0] ;
         var clName = clFullName.split( "." )[1] ;
         var groups = obj.Groups ;

         for ( var i in groups )
         {
            var nodeList = groups[i].NodeList ;
            var groupName = groups[i].GroupName ;

            for ( var j in nodeList )
            {
               var info = {} ;
               var arr = nodeList[j].split( ":" ) ;
               var hostname = arr[0] ;
               var svcname = arr[1] ;
               var dataDB = new Sdb( hostname, svcname, USER ) ;
               var collection = dataDB.getCS( csName ).getCL( clName ) ;
               var clUniqeID = 0 ;
               var recordCount  = collection.count().valueOf() ;
               var lobCount = collection.listLobs().size() ;
               var sqlStr2 = "select UniqueID from $SNAPSHOT_CL where Name='" + clFullName + "'" ;

               /*
               {
                  "ClFullName": "cs.cl",
                  "NodeName": 192.168.17.50:11820,
                  "GroupName": "group1",
                  "UniqueID": 123456789,
                  "InvalidType": "Local" or "Residual",
                  "RecordCount": 10000,
                  "LobCount": 100
               }
               */

               var rc1 = dataDB.exec( sqlStr2 ) ;
               if ( rc1.next() )
               {
                  clUniqeID = rc1.current().toObj().UniqueID ;
               }
               else
               {
                  println( "(" + step + "/" + STEP_NUM + ")Can't find " + clFullName +
                           " cl UniqueID from $SNAPSHOT_CL" ) ;
               }

               info = { "ClFullName": clFullName, "NodeName": hostname + ":" + svcname,
                        "GroupName": groupName, "UniqueID": clUniqeID,
                        "RecordCount": recordCount, "LobCount": lobCount,
                        "InvalidType": isLocalID( clUniqeID ) ? "Local" : "Residual" } ;

               invalidClInfoCl.insert( info ) ;

               dataDB.close() ;
            }
         }
      }

      println( "(" + step + "/" + STEP_NUM + ")End to collect invalid collection inofs, spent time: " +
               ( (Date.now()) - beginTime )/1000 + "s" ) ;
      step++ ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to collect invalid collection inofs, error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

function collect()
{
   collectNodesInfo() ;

   collectCataClusterClInfo() ;

   collectCataMainClInfo() ;

   collectDataClusterClInfo() ;

   collectLocalClInfo() ;

   collectCataIndexInfo() ;

   collectDataIndexInfoConcurrent() ;

   collectInvalidClInfos() ;
}

function aggregateIndexInfo()
{
   /*

   聚集，获取每个集合每个索引的索引信息和节点信息

   tmp_cl_data_index_check_info

   {
      "GroupName": "db3",
      "IndexDef": {
         "UniqueID": 390842023936,
         "key": {
            "_id": 1
         },
         "v": 0,
         ...
      },
      "ClFullName": "cs.cl",
      "MainClName": null,
      "DataClFullName": "cs.cl",
      "IndexName": [
         [
            "$id"
         ]
      ],
      "Groups": [
         {
            "GroupName": "db3",
            "NodeInfos": [
            {
               "NodeName": "u16-fjb:25000",
               "IndexName": "$id",
               "UniqueID": 390842023936
            },
            ...
            ]
         }
      ],
      "NodeCount": 3,
      "GroupCount": 1,
      "UniqueIDs": [
         390842023936
      ],
      "UniqueIDCount": 1,
      "IndexDataType": 1
   }

   tmp_cl_data_index_check_info_tmp

   {
      "GroupName": "db3",
      "ClFullName": "cs.cl",
      "MainClName": null,
      "DataClFullName": "cs.cl",
      "IndexName": "$id",
      "IndexDefs": [
         [
            {
               "UniqueID": 390842023936,
               "key": {
                  "_id": 1
               },
               "v": 0,
               ...
            }
         ]
      ],
      "Groups": [
         {
            "GroupName": "db3",
            "NodeInfos": [
            {
               "NodeName": "u16-fjb:25000",
               "IndexName": "$id",
               "UniqueID": 390842023936
            },
            ...
            ]
         }
      ],
      "NodeCount": 3,
      "GroupCount": 1,
      "IndexDataType": 1
   }

   */

   try
   {
      var beginTime = Date.now() ;

      var t1SqlStr1 = "select GroupName,IndexDef,ClFullName,MainClName,DataClFullName,IndexDataType,addtoset(IndexName) as IndexNames,addtoset(UniqueID) as UniqueIDs,addtoset(NodeInfo) as NodeInfos,count(NodeInfo) as NodeCount from " + TMP_CL_FULL_DATA_INDEX_INFO + " group by GroupName,ClFullName,IndexDef" ;

      var t2SqlStr1 = "select T1.GroupName as GroupName,T1.IndexDef as IndexDef,T1.ClFullName as ClFullName,T1.MainClName as MainClName,T1.DataClFullName as DataClFullName,T1.IndexDataType as IndexDataType,T1.IndexNames as IndexNames,T1.UniqueIDs as UniqueIDs,T1.NodeCount as NodeCount,buildobj(T1.GroupName,T1.NodeInfos) as Group from ( " + t1SqlStr1 + " ) as T1 group by T1.GroupName,T1.ClFullName,T1.IndexDef" ;

      var t3SqlStr1 = "select T2.GroupName as GroupName,T2.IndexDef as IndexDef,T2.ClFullName as ClFullName,T2.MainClName as MainClName,T2.DataClFullName as DataClFullName,T2.IndexDataType as IndexDataType,addtoset(T2.IndexNames) as IndexNames,addtoset(T2.UniqueIDs) as UniqueIDs,addtoset(T2.Group) as Groups,sum(T2.NodeCount) as NodeCount,count(T2.Group) as GroupCount from ( " + t2SqlStr1 + " ) as T2 group by T2.ClFullName,T2.IndexDef" ;

      var t4SqlStr1 = "select * from ( " + t3SqlStr1 + " ) as T3 split by T3.UniqueIDs" ;

      var t5SqlStr1 = "select * from ( " + t4SqlStr1 + " ) as T4 split by T4.UniqueIDs" ;

      var sqlStr1 = "insert into " + TMP_CL_FULL_DATA_INDEX_CHECK_INFO + " select T5.GroupName as GroupName,T5.IndexDef as IndexDef,T5.ClFullName as ClFullName,T5.MainClName as MainClName,T5.DataClFullName as DataClFullName,T5.IndexDataType as IndexDataType,T5.IndexNames as IndexNames,T5.Groups as Groups,T5.NodeCount as NodeCount,T5.GroupCount as GroupCount,addtoset(T5.UniqueIDs) as UniqueIDs,count(T5.UniqueIDs) as UniqueIDCount from ( " + t5SqlStr1 + " ) as T5 group by T5.ClFullName,T5.IndexDef" ;

      println( "(" + step + "/" + STEP_NUM + ")Begin to aggregate data index infos( GROUP BY: GroupName, ClFullName, IndexDef )" );
      db.execUpdate( sqlStr1 ) ;
      println( "(" + step + "/" + STEP_NUM + ")End to aggregate data index infos( GROUP BY: GroupName, ClFullName, IndexDef ), spent time: " +
               ( (Date.now()) - beginTime )/1000 + "s" ) ;
      beginTime = Date.now() ;
      step++ ;

      println( "(" + step + "/" + STEP_NUM + ")Begin to update uniqueID infos" ) ;
      var sqlStr2 = "update " + TMP_CL_FULL_DATA_INDEX_CHECK_INFO + " set UniqueIDCount=0 where UniqueIDs=''" ;
      db.execUpdate( sqlStr2 ) ;
      println( "(" + step + "/" + STEP_NUM + ")End to update uniqueID infos, spent time: " +
               ( (Date.now()) - beginTime )/1000 + "s" ) ;
      beginTime = Date.now() ;
      step++ ;

      var t1SqlStr3 = "select GroupName,ClFullName,MainClName,DataClFullName,IndexName,addtoset(IndexDef) as IndexDefs,addtoset(NodeInfo) as NodeInfos,count(NodeInfo) as NodeCount from " + TMP_CL_FULL_DATA_INDEX_INFO + " group by GroupName,ClFullName,IndexName" ;
      var t2SqlStr3 = "select T1.GroupName as GroupName,T1.ClFullName as ClFullName,T1.MainClName as MainClName,T1.DataClFullName as DataClFullName,T1.IndexName as IndexName,T1.IndexDefs as IndexDefs,T1.NodeCount as NodeCount,buildobj(T1.GroupName,T1.NodeInfos) as Group from ( " + t1SqlStr3 + " ) as T1 group by T1.GroupName,T1.ClFullName,T1.IndexName" ;

      var sqlStr3 = "insert into " + TMP_CL_FULL_DATA_INDEX_CHECK_INFO_TMP + " select T2.GroupName as GroupName,T2.ClFullName as ClFullName,T2.MainClName as MainClName,T2.DataClFullName as DataClFullName,T2.IndexName as IndexName,addtoset(T2.IndexDefs) as IndexDefs,addtoset(T2.Group) as Groups,sum(T2.NodeCount) as NodeCount,count(T2.Group) as GroupCount from ( " + t2SqlStr3 + " ) as T2 group by T2.ClFullName,T2.IndexName" ;

      println( "(" + step + "/" + STEP_NUM + ")Begin to aggregate data index infos( GROUP BY: GroupName, ClFullName, IndexName )" );
      db.execUpdate( sqlStr3 ) ;
      println( "(" + step + "/" + STEP_NUM + ")End to aggregate data index infos( GROUP BY: GroupName, ClFullName, IndexName ), spent time: " +
               ( (Date.now()) - beginTime )/1000 + "s" ) ;
      step++ ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to check aggregate data index infos, error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

function checkConflictSameDefIndex()
{
   try
   {
      var beginTime = Date.now() ;
      var cannotUpgradeIdxInfoCl = db.getCS( TMP_CS_UPGRADE_INDEX ).getCL( TMP_CL_CANNOT_UPGRADE_INDEX_INFO ) ;

      // 同定义不同名的冲突索引
      var t1SqlStr1 = "select * from " + TMP_CL_FULL_DATA_INDEX_CHECK_INFO + " split by IndexNames" ;
      var t2SqlStr1 = "select * from ( " + t1SqlStr1 + " ) as T1 split by T1.IndexNames" ;
      var t3SqlStr1 = "select T2.GroupName as GroupName,T2.IndexDef as IndexDef,T2.ClFullName as ClFullName,T2.MainClName as MainClName,T2.DataClFullName as DataClFullName,T2.Groups as Groups,T2.NodeCount as NodeCount,T2.GroupCount as GroupCount,T2.UniqueIDs as UniqueIDs,T2.UniqueIDCount as UniqueIDCount,addtoset(T2.IndexNames) as IndexNames,count(T2.IndexNames) as IndexNameCount from ( " + t2SqlStr1 + " ) as T2 group by T2.ClFullName,T2.IndexDef" ;

      println( "(" + step + "/" + STEP_NUM + ")Begin to check conflict indexes( same index def, diff index name )" ) ;
      var sqlStr1 = "select * from ( " + t3SqlStr1 + " ) as T3 where T3.IndexNameCount > 1" ;
      var batchRecords = [] ;
      var rc = db.exec( sqlStr1 ) ;
      while ( rc.next() )
      {
         var obj = rc.current().toObj() ;

         obj.UpgradeIndexType = IDX_TYPE_CONFLICT ;

         batchRecords.push( obj ) ;

         if ( batchRecords.length > BATCH_NUM )
         {
            cannotUpgradeIdxInfoCl.insert( batchRecords ) ;
            batchRecords = [] ;
         }
      }
      if ( batchRecords.length > 0 )
      {
         cannotUpgradeIdxInfoCl.insert( batchRecords ) ;
         batchRecords = [] ;
      }
      println( "(" + step + "/" + STEP_NUM + ")End to check conflict indexes( same index def, diff index name ), spent time: " +
               ( (Date.now()) - beginTime )/1000 + "s" ) ;
      step++ ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to check conflict indexes( same index def, diff index name ), error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

function checkConflictSameNameIndex()
{
   try
   {
      var beginTime = Date.now() ;
      var cannotUpgradeIdxInfoCl = db.getCS( TMP_CS_UPGRADE_INDEX ).getCL( TMP_CL_CANNOT_UPGRADE_INDEX_INFO ) ;

      // 同名不同定义的冲突索引
      var t1SqlStr1 = "select * from " + TMP_CL_FULL_DATA_INDEX_CHECK_INFO_TMP + " split by IndexDefs" ;
      var t2SqlStr1 = "select * from ( " + t1SqlStr1 + " ) as T1 split by T1.IndexDefs" ;
      var t3SqlStr1 = "select T2.GroupName as GroupName,T2.ClFullName as ClFullName,T2.MainClName as MainClName,T2.DataClFullName as DataClFullName,T2.IndexName as IndexName,T2.Groups as Groups,T2.NodeCount as NodeCount,T2.GroupCount as GroupCount,addtoset(T2.IndexDefs) as IndexDefs,count(T2.IndexDefs) as IndexDefCount from ( " + t2SqlStr1 + " ) as T2 group by T2.ClFullName,T2.IndexName" ;

      var sqlStr1 = "select * from ( " + t3SqlStr1 + " ) as T3 where T3.IndexDefCount > 1" ;

      println( "(" + step + "/" + STEP_NUM + ")Begin to check conflict indexes( diff index def, same index name )" ) ;
      var batchRecords = [] ;
      var rc = db.exec( sqlStr1 ) ;
      while ( rc.next() )
      {
         var obj = rc.current().toObj() ;

         obj.UpgradeIndexType = IDX_TYPE_CONFLICT ;

         batchRecords.push( obj ) ;

         if ( batchRecords.length > BATCH_NUM )
         {
            cannotUpgradeIdxInfoCl.insert( batchRecords ) ;
            batchRecords = [] ;
         }
      }
      if ( batchRecords.length > 0 )
      {
         cannotUpgradeIdxInfoCl.insert( batchRecords ) ;
         batchRecords = [] ;
      }

      println( "(" + step + "/" + STEP_NUM + ")End to check conflict indexes( diff index def, same index name ), spent time: " +
               ( (Date.now()) - beginTime )/1000 + "s" ) ;
      step++ ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to check conflict indexes( diff index def, same index name ), error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

function swapTmpClName( clName1, clName2 )
{
   try
   {
      var tmpCS = db.getCS( TMP_CS_UPGRADE_INDEX ) ;

      tmpCS.renameCL( clName1, "tmp" ) ;
      tmpCS.renameCL( clName2, clName1 ) ;
      tmpCS.renameCL( "tmp", clName2 ) ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to swap tmp cl name, error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

function checkConflictIndex()
{
   try
   {
      var beginTime = Date.now() ;
      var tmpCS = db.getCS( TMP_CS_UPGRADE_INDEX ) ;
      var cacheInfoCl = tmpCS.getCL( TMP_CL_CACHE_INFO ) ;
      var dataIndexCheckInfoDefCl = tmpCS.getCL( TMP_CL_DATA_INDEX_CHECK_INFO ) ;
      var cataIdxInfo = tmpCS.getCL( TMP_CL_CATA_INDEX_INFO ) ;

      checkConflictSameDefIndex() ;

      checkConflictSameNameIndex() ;

      println( "(" + step + "/" + STEP_NUM + ")Begin to clear conflict indexes info" ) ;
      var sqlStr1 = "select * from " + TMP_CL_FULL_CANNOT_UPGRADE_INDEX_INFO + " where UpgradeIndexType='" + IDX_TYPE_CONFLICT + "'" ;
      rc = db.exec( sqlStr1 ) ;
      while ( rc.next() )
      {
         var obj = rc.current().toObj() ;

         if ( undefined != obj.IndexName )
         {
            cataIdxInfo.remove( { "ClFullName": obj.ClFullName, "IndexName": obj.IndexName } ) ;
            dataIndexCheckInfoDefCl.remove( { "ClFullName": obj.ClFullName, "IndexNames": [ obj.IndexName ] } ) ;
         }
         else if ( undefined != obj.IndexNames )
         {
            for ( i in obj.IndexNames )
            {
               var idxName = obj.IndexNames[i]
               cataIdxInfo.remove( { "ClFullName": obj.ClFullName, "IndexName": idxName } ) ;
            }
            dataIndexCheckInfoDefCl.remove( { "ClFullName": obj.ClFullName, "IndexDef": obj.IndexDef } ) ;
         }
      }

      /*

      split by IndexNames

      {
         ...
         IndexNames: [ [ "$id" ] ],
         ...
      }

      change to

      {
         ...
         IndexNames: "$id",
         ...
      }

      */
      var t1SqlStr2 = "select * from " + TMP_CL_FULL_DATA_INDEX_CHECK_INFO + " split by IndexNames" ;

      var sqlStr2 = "insert into " + TMP_CL_FULL_CACHE_INFO + " select T1.GroupName as GroupName,T1.IndexDef as IndexDef,T1.ClFullName as ClFullName,T1.MainClName as MainClName,T1.DataClFullName as DataClFullName,T1.IndexDataType as IndexDataType,T1.IndexNames as IndexName,T1.Groups as Groups,T1.NodeCount as NodeCount,T1.GroupCount as GroupCount,T1.UniqueIDs as UniqueIDs,T1.UniqueIDCount as UniqueIDCount from ( " + t1SqlStr2 + " ) as T1 split by T1.IndexNames" ;

      truncateCL( cacheInfoCl ) ;
      cacheInfoCl.createAutoIncrement( { Field: "AutoIncrementFiled" } )
      db.execUpdate( sqlStr2 ) ;

      swapTmpClName( TMP_CL_CACHE_INFO, TMP_CL_DATA_INDEX_CHECK_INFO ) ;

      println( "(" + step + "/" + STEP_NUM + ")End to clear conflict indexes info, spent time: " +
               ( (Date.now()) - beginTime )/1000 + "s" ) ;
      step++ ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to check conflict indexes( diff index def, same index name ), error Stack:\n" + e.stack ) ;
      }
      throw e ;
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

function checkMissIndex()
{
   try
   {
      var rc = null ;
      var beginTime = Date.now() ;
      var tmpCS = db.getCS( TMP_CS_UPGRADE_INDEX ) ;
      var cacheInfoCl = tmpCS.getCL( TMP_CL_CACHE_INFO ) ;
      var dataIndexCheckInfoDefCl = tmpCS.getCL( TMP_CL_DATA_INDEX_CHECK_INFO ) ;
      var cataIdxInfo = tmpCS.getCL( TMP_CL_CATA_INDEX_INFO ) ;
      var cannotUpgradeIdxInfoCl = tmpCS.getCL( TMP_CL_CANNOT_UPGRADE_INDEX_INFO ) ;
      var noNeedUpgradeIdxInfoCl = tmpCS.getCL( TMP_CL_NO_NEED_UPGRADE_INDEX_INFO ) ;

      println( "(" + step + "/" + STEP_NUM + ")Begin to collect consistent indexes" ) ;
      // 获取编目和数据一致的索引
      var t1SqlStr1 = "select ClFullName,MainClName,EnsureShardingIndex,ShardingKey,Attribute,Groups,GroupCount,NodeCount,buildobj(ClFullName,GroupCount,NodeCount) as JoinMatch from " + TMP_CL_FULL_CATA_CLUSTER_CL_INFO + " group by ClFullName" ;
      var t2SqlStr1 = "select GroupName,IndexDef,ClFullName,MainClName,DataClFullName,IndexDataType,IndexName,Groups,NodeCount,GroupCount,UniqueIDs,UniqueIDCount,AutoIncrementFiled,buildobj(ClFullName,GroupCount,NodeCount) as JoinMatch from " + TMP_CL_FULL_DATA_INDEX_CHECK_INFO + " group by ClFullName,IndexName" ;

      var sqlStr1 = "insert into " + TMP_CL_FULL_CACHE_INFO + " select T1.ClFullName as CataClFullName,T1.MainClName,T1.EnsureShardingIndex,T1.ShardingKey,T1.Attribute,T1.Groups as CataGroups,T1.GroupCount,T1.NodeCount,T2.ClFullName,T2.DataClFullName,T2.GroupName,T2.Groups as DataGroups,T2.IndexDef,T2.IndexName as IndexName,T2.UniqueIDCount,T2.UniqueIDs,T2.IndexDataType,T2.AutoIncrementFiled from ( " + t1SqlStr1 + " ) as T1 inner join ( " + t2SqlStr1 + " ) as T2 on T1.JoinMatch=T2.JoinMatch /*+use_hash()*/" ;

      truncateCL( cacheInfoCl ) ;
      db.execUpdate( sqlStr1 ) ;
      println( "(" + step + "/" + STEP_NUM + ")End to collect consistent indexes, spent time: " +
               ( (Date.now()) - beginTime )/1000 + "s" ) ;
      beginTime = Date.now() ;
      step++ ;

      // 删除一致的索引，剩下的就是缺失索引
      println( "(" + step + "/" + STEP_NUM + ")Begin to collect miss index infos" ) ;
      dataIndexCheckInfoDefCl.createIndex( "autoIncrementFiled_idx", { "AutoIncrementFiled": 1 } ) ;
      var sqlStr2 = "select AutoIncrementFiled from " + TMP_CL_FULL_CACHE_INFO ;
      rc = db.exec( sqlStr2 ) ;
      var batchRecords = [] ;
      while ( rc.next() )
      {
         batchRecords.push( rc.current().toObj().AutoIncrementFiled ) ;

         if ( batchRecords.length > BATCH_NUM )
         {
            dataIndexCheckInfoDefCl.remove( { "AutoIncrementFiled": { "$in": batchRecords } } ) ;
            batchRecords = [] ;
         }
      }
      if ( batchRecords.length > 0 )
      {
         dataIndexCheckInfoDefCl.remove( { "AutoIncrementFiled": { "$in": batchRecords } } ) ;
         batchRecords = [] ;
      }

      println( "(" + step + "/" + STEP_NUM + ")End to collect miss index infos, spent time: " +
               ( (Date.now()) - beginTime )/1000 + "s" ) ;
      beginTime = Date.now() ;
      step++ ;

      println( "(" + step + "/" + STEP_NUM + ")Begin to clear miss index infos" ) ;
      var sqlStr2 = "select * from " + TMP_CL_FULL_DATA_INDEX_CHECK_INFO ;
      rc = db.exec( sqlStr2 ) ;
      while ( rc.next() )
      {
         var obj = rc.current().toObj() ;
         var standaloneIdx = false ;

         cataIdxInfo.remove( { "ClFullName": obj.ClFullName, "IndexName": obj.IndexName } ) ;

         if ( obj.UniqueIDCount > 0 )
         {
            for ( i in obj.UniqueIDs )
            {
               if ( isLocalID( obj.UniqueIDs[i] ) )
               {
                  standaloneIdx = true ;
                  break ;
               }
            }
         }

         if ( standaloneIdx )
         {
            obj.UpgradeIndexType = IDX_TYPE_STANDALONE ;
            noNeedUpgradeIdxInfoCl.insert( obj ) ;
         }
         else
         {
            obj.UpgradeIndexType = IDX_TYPE_MISSING ;
            cannotUpgradeIdxInfoCl.insert( obj ) ;
         }
      }

      swapTmpClName( TMP_CL_CACHE_INFO, TMP_CL_DATA_INDEX_CHECK_INFO ) ;

      println( "(" + step + "/" + STEP_NUM + ")End to clear miss index infos, spent time: " +
               ( (Date.now()) - beginTime )/1000 + "s" ) ;
      step++ ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to check miss indexes, error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

function checkInvalidIdAndShardIdx()
{
   try
   {
      var rc = null ;
      var beginTime = Date.now() ;

      println( "(" + step + "/" + STEP_NUM + ")Begin to check invalid $id and $shard indexes" ) ;

      var tmpCS = db.getCS( TMP_CS_UPGRADE_INDEX ) ;
      var dataIndexCheckInfoDefCl = tmpCS.getCL( TMP_CL_DATA_INDEX_CHECK_INFO ) ;
      var cataIdxInfo = tmpCS.getCL( TMP_CL_CATA_INDEX_INFO ) ;
      var cannotUpgradeIdxInfoCl = tmpCS.getCL( TMP_CL_CANNOT_UPGRADE_INDEX_INFO ) ;

      dataIndexCheckInfoDefCl.update( { "$bit": { "Attribute": { "and": MASK_CLATTR_NOIDIDX } } }, { "IndexName": "$id" } ) ;

      dataIndexCheckInfoDefCl.update( { "$set": { "UpgradeIndexType": IDX_TYPE_INVALID_SHARD } }, { "IndexName": "$shard", "EnsureShardingIndex": false } ) ;

      dataIndexCheckInfoDefCl.update( { "$set": { "UpgradeIndexType": IDX_TYPE_INVALID_ID } }, { "IndexName": "$id", "Attribute": { "$ne": 0 } } ) ;

      rc = dataIndexCheckInfoDefCl.find( { "UpgradeIndexType": { "$in": [ IDX_TYPE_INVALID_SHARD, IDX_TYPE_INVALID_ID ] } } ) ;
      while ( rc.next() )
      {
         var obj = rc.current().toObj() ;
         dataIndexCheckInfoDefCl.remove( { "ClFullName": obj.ClFullName, "IndexName": obj.IndexName } ) ;
         cataIdxInfo.remove( { "ClFullName": obj.ClFullName, "IndexName": obj.IndexName } ) ;
         cannotUpgradeIdxInfoCl.insert( obj ) ;
      }

      println( "(" + step + "/" + STEP_NUM + ")End to check invalid $id and $shard indexes, spent time: " +
               ( (Date.now()) - beginTime )/1000 + "s" ) ;
      step++ ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to check invalid $id and $shard indexes, error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

function checkCataIndexMetaData()
{
   try
   {
      var beginTime = Date.now() ;

      println( "(" + step + "/" + STEP_NUM + ")Begin to check index cata meta infos" ) ;

      var tmpCS = db.getCS( TMP_CS_UPGRADE_INDEX ) ;
      var cacheInfoCl = tmpCS.getCL( TMP_CL_CACHE_INFO ) ;

      var sqlStr1 = "insert into " + TMP_CL_FULL_DATA_INDEX_CHECK_INFO + " select * from (select T1.MainClName,T2.ClFullName, T2.IndexName, T2.IndexDataType from " + TMP_CL_FULL_CATA_CLUSTER_CL_INFO + " as T1 inner join " + TMP_CL_FULL_CATA_INDEX_INFO + " as T2 on T1.ClFullName=T2.ClFullName /*+use_hash()*/) as T3" ;
      db.execUpdate( sqlStr1 ) ;

      var sqlStr2 = "insert into " + TMP_CL_FULL_CACHE_INFO + " select ClFullName,MainClName,max(IndexDef) as IndexDef,IndexName,max(IndexDataType) as IndexDataType,max(GroupName) as GroupName,max(EnsureShardingIndex) as EnsureShardingIndex,max(ShardingKey) as ShardingKey,max(Attribute) as Attribute,max(DataGroups) as Groups,max(NodeCount) as NodeCount,max(GroupCount) as GroupCount,max(UniqueIDCount) as UniqueIDCount,max(UniqueIDs) as UniqueIDs from " + TMP_CL_FULL_DATA_INDEX_CHECK_INFO + " group by ClFullName,IndexName" ;
      truncateCL( cacheInfoCl ) ;
      db.execUpdate( sqlStr2 ) ;

      swapTmpClName( TMP_CL_CACHE_INFO, TMP_CL_DATA_INDEX_CHECK_INFO ) ;

      println( "(" + step + "/" + STEP_NUM + ")End to check index cata meta infos, spent time: " +
               ( (Date.now()) - beginTime )/1000 + "s" ) ;
      step++ ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to check index cata meta infos, error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

function checkCanUpgradeAndStandaloneIdx()
{
   try
   {
      var rc = null ;
      var beginTime = Date.now() ;
      /*
      IndexDataType != 0 表示编目有元数据

      IndexDataType != 0 and UniqueIDCount = 1,  IDX_TYPE_CONSISTENT
      IndexDataType != 0 and UniqueIDCount != 1, IDX_TYPE_CAN_UPGRADE

      IndexDataType = 0 表示编目没有元数据

      IndexDataType = 0 and UniqueIDCount = 0,  IDX_TYPE_CAN_UPGRADE
      IndexDataType = 0 and UniqueIDCount != 0, IDX_TYPE_STANDALONE or IDX_TYPE_CAN_UPGRADE
      */

      println( "(" + step + "/" + STEP_NUM + ")Begin to check can upgrade indexes and standalone indexes" ) ;

      var tmpCS = db.getCS( TMP_CS_UPGRADE_INDEX ) ;
      var dataIdxCheckInfoCl = tmpCS.getCL( TMP_CL_DATA_INDEX_CHECK_INFO ) ;
      var noNeedUpgradeIdxInfoCl = tmpCS.getCL( TMP_CL_NO_NEED_UPGRADE_INDEX_INFO ) ;

      var sqlStr1 = "update " + TMP_CL_FULL_DATA_INDEX_CHECK_INFO + " set UpgradeIndexType='" + IDX_TYPE_CONSISTENT + "' where IndexDataType<>0 and UniqueIDCount=1" ;
      db.execUpdate( sqlStr1 ) ;

      var sqlStr2 = "select * from " + TMP_CL_FULL_DATA_INDEX_CHECK_INFO + " where IndexDataType=0 and UniqueIDCount<>0" ;
      rc = db.exec( sqlStr2 ) ;
      while ( rc.next() )
      {
         var obj = rc.current().toObj() ;

         for ( i in obj.UniqueIDs )
         {
            if ( isLocalID( obj.UniqueIDs[i] ) )
            {
               obj.UpgradeIndexType = IDX_TYPE_STANDALONE ;
               noNeedUpgradeIdxInfoCl.insert( obj ) ;
               break ;
            }
         }
      }

      var sqlStr3 = "select * from " + TMP_CL_FULL_NO_NEED_UPGRADE_INDEX_INFO + " where UpgradeIndexType='" + IDX_TYPE_STANDALONE + "'" ;
      rc = db.exec( sqlStr3 ) ;
      while ( rc.next() )
      {
         var obj = rc.current().toObj() ;
         dataIdxCheckInfoCl.remove( { "ClFullName": obj.ClFullName, "IndexName": obj.IndexName } ) ;
      }

      println( "(" + step + "/" + STEP_NUM + ")End to check can upgrade indexes and standalone indexes, spent time: " +
               ( (Date.now()) - beginTime )/1000 + "s" ) ;
      step++ ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to check can upgrade indexes and standalone indexes, error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

function checkUniqueIdxKey( uniqueIdxKey, shardingKey )
{
   for ( let key in shardingKey )
   {
      if ( uniqueIdxKey[key] !== null && uniqueIdxKey[key] == shardingKey[key] )
      {
         continue ;
      }
      else
      {
         return false ;
      }
   }
   return true ;
}

// All fields in sharding key must be included in unique index
function checkClusterClInvalidUniqueIdx()
{
   try
   {
      var rc = null ;
      var beginTime = Date.now() ;
      println( "(" + step + "/" + STEP_NUM + ")Begin to check invalid unique indexes" ) ;

      var tmpCS = db.getCS( TMP_CS_UPGRADE_INDEX ) ;
      var dataIdxCheckInfoCl = tmpCS.getCL( TMP_CL_DATA_INDEX_CHECK_INFO ) ;
      var cannotUpgradeIdxInfoCl = tmpCS.getCL( TMP_CL_CANNOT_UPGRADE_INDEX_INFO ) ;

      rc = dataIdxCheckInfoCl.find( { "IndexDef.unique": true,
                                      "IndexName": { "$ne": "$id" },
                                      "ShardingKey": { "$isnull": 0 } },
                                    { "ClFullName": 1, "IndexDef": 1,
                                      "IndexName": 1, "ShardingKey": 1 } ) ;
      while ( rc.next() )
      {
         var obj = rc.current().toObj() ;
         if ( !checkUniqueIdxKey( obj.IndexDef.key, obj.ShardingKey ) )
         {
            obj.UpgradeIndexType = IDX_TYPE_INVALID_UNI ;
            cannotUpgradeIdxInfoCl.insert( obj ) ;
         }
      }

      rc = cannotUpgradeIdxInfoCl.find( { "UpgradeIndexType": IDX_TYPE_INVALID_UNI },
                                        { "ClFullName": 1, "IndexDef": 1,
                                          "IndexName": 1, "ShardingKey": 1 } ) ;
      while ( rc.next() )
      {
         var obj = rc.current().toObj() ;
         dataIdxCheckInfoCl.remove( obj ) ;
      }

      println( "(" + step + "/" + STEP_NUM + ")End to check invalid unique indexes, spent time: " +
               ( (Date.now()) - beginTime )/1000 + "s" ) ;
      step++ ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to check invalid unique indexes, error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

function checkClusterClIndexInfo()
{
   aggregateIndexInfo() ;

   checkConflictIndex() ;

   checkMissIndex() ;

   checkInvalidIdAndShardIdx() ;

   checkCataIndexMetaData() ;

   checkCanUpgradeAndStandaloneIdx() ;

   checkClusterClInvalidUniqueIdx() ;
}

function checkMainClCanUpgradeIndexInfo()
{
   try
   {
      var beginTime = Date.now() ;
      var cacheInfoCl = db.getCS( TMP_CS_UPGRADE_INDEX ).getCL( TMP_CL_CACHE_INFO ) ;
      var mainClInfoCl = db.getCS( TMP_CS_UPGRADE_INDEX ).getCL( TMP_CL_CATA_MAIN_CL_INFO ) ;

      println( "(" + step + "/" + STEP_NUM + ")Begin to check main collection index infos" ) ;

      truncateCL( cacheInfoCl ) ;

      if ( 0 == mainClInfoCl.count() )
      {
         println( "(" + step + "/" + STEP_NUM + ")No Main Collections" ) ;
      }
      else
      {
         var t2SqlStr1 = "select IndexName,EnsureShardingIndex,IndexDef,MainClName,count(ClFullName) as SubCLCount,addtoset(ShardingKey) as SubClShardingKeys from " + TMP_CL_FULL_DATA_INDEX_CHECK_INFO + " group by MainClName,IndexName" ;
         var t3SqlStr1 = "select T2.IndexName as IndexName,T2.EnsureShardingIndex as EnsureShardingIndex,T2.IndexDef as IndexDef,T2.MainClName as MainClName,T2.SubClShardingKeys as SubClShardingKeys, T2.SubCLCount as SubCLCount, T1.ShardingKey as ShardingKey from " + TMP_CL_FULL_CATA_MAIN_CL_INFO + " as T1 inner join ( " + t2SqlStr1 + " ) as T2 on T1.Name=T2.MainClName and T1.SubCLCount=T2.SubCLCount" ;

         var sqlStr1 = "insert into " + TMP_CL_FULL_CACHE_INFO + " select T3.EnsureShardingIndex as EnsureShardingIndex,T3.MainClName as ClFullName,T3.SubClShardingKeys as SubClShardingKeys,T3.IndexName as IndexName,T3.IndexDef as IndexDef, count(T3.IndexName) as IndexDataType, T3.SubCLCount as SubCLCount, T3.ShardingKey as ShardingKey from ( " + t3SqlStr1 + " ) as T3 group by T3.MainClName,T3.IndexName" ;
         db.execUpdate( sqlStr1 ) ;

         var sqlStr2 = "insert into " + TMP_CL_FULL_CACHE_INFO + " select * from ( select T2.ClFullName, T2.IndexName, T2.IndexDataType from " + TMP_CL_FULL_CATA_MAIN_CL_INFO + " as T1 inner join " + TMP_CL_FULL_CATA_INDEX_INFO + " as T2 on T1.Name=T2.ClFullName /*+use_hash()*/ ) as T3" ;
         db.execUpdate( sqlStr2 ) ;

         var sqlStr3 = "insert into " + TMP_CL_FULL_MAIN_CL_INDEX_CHECK_INFO + " select ClFullName,IndexName,max(IndexDataType) as IndexDataType,max(EnsureShardingIndex) as EnsureShardingIndex,max(SubClShardingKeys) as SubClShardingKeys,max(IndexDef) as IndexDef,max(SubCLCount) as SubCLCount, max(ShardingKey) as ShardingKey from " + TMP_CL_FULL_CACHE_INFO + " group by ClFullName,IndexName" ;
         db.execUpdate( sqlStr3 ) ;

         var sqlStr4 = "delete from " + TMP_CL_FULL_MAIN_CL_INDEX_CHECK_INFO + " where ClFullName is null or IndexName is null" ;
         db.execUpdate( sqlStr4 ) ;

         var sqlStr5 = "delete from " + TMP_CL_FULL_MAIN_CL_INDEX_CHECK_INFO + " where IndexName='$id' or IndexName='$shard'"
         db.execUpdate( sqlStr5 ) ;

         var sqlStr6 = "update " + TMP_CL_FULL_MAIN_CL_INDEX_CHECK_INFO + " set UpgradeIndexType='" + IDX_TYPE_CONSISTENT + "' where IndexDataType<>1" ;
         db.execUpdate( sqlStr6 ) ;
      }

      println( "(" + step + "/" + STEP_NUM + ")End to check main collection index infos, spent time: " +
               ( (Date.now()) - beginTime )/1000 + "s" ) ;
      step++ ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to check main collection index infos, error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

function checkMainClInvalidUniqueIdx()
{
   try
   {
      var beginTime = Date.now() ;
      var tmpCS = db.getCS( TMP_CS_UPGRADE_INDEX ) ;
      var mainClInfoCl = tmpCS.getCL( TMP_CL_MAIN_CL_INDEX_CHECK_INFO ) ;
      var cannotUpgradeIdxInfoCl = tmpCS.getCL( TMP_CL_CANNOT_UPGRADE_INDEX_INFO ) ;

      println( "(" + step + "/" + STEP_NUM + ")Begin to check main collection invalid unique indexes" ) ;

      rc = mainClInfoCl.find( { "IndexDef.unique": true,
                                "IndexName": { "$ne": "$id" },
                                "ShardingKey": { "$isnull": 0 } } ) ;
      while ( rc.next() )
      {
         var obj = rc.current().toObj() ;
         var subClShardingKeys = obj.SubClShardingKeys ;
         var includeAllSubShardingKey = true ;
         // unique idx must include all subcl shardingKey fields
         for ( let i = 0 ; i < subClShardingKeys.length ; ++i )
         {
            if ( !checkUniqueIdxKey( obj.IndexDef.key, subClShardingKeys[i] ) )
            {
               includeAllSubShardingKey = false ;
               break ;
            }
         }
         // unique idx must include maincl shardingKey fields
         if ( !includeAllSubShardingKey ||
              !checkUniqueIdxKey( obj.IndexDef.key, obj.ShardingKey ) )
         {
            obj.UpgradeIndexType = IDX_TYPE_INVALID_UNI ;
            cannotUpgradeIdxInfoCl.insert( obj ) ;
         }
      }

      rc = cannotUpgradeIdxInfoCl.find( { "UpgradeIndexType": IDX_TYPE_INVALID_UNI },
                                        { "ClFullName": 1, "IndexDef": 1,
                                          "IndexName": 1, "ShardingKey": 1 } ) ;
      while ( rc.next() )
      {
         var obj = rc.current().toObj() ;
         mainClInfoCl.remove( obj ) ;
      }

      println( "(" + step + "/" + STEP_NUM + ")End to check main collection invalid unique indexes, spent time: " +
               ( (Date.now()) - beginTime )/1000 + "s" ) ;
      step++ ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to check main collection invalid unique indexes, error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

function checkMainClIndexInfo()
{
   checkMainClCanUpgradeIndexInfo() ;

   checkMainClInvalidUniqueIdx() ;
}

function checkLocalClIndexInfo()
{
   try
   {
      var beginTime = Date.now() ;

      println( "(" + step + "/" + STEP_NUM + ")Begin to check local collection index inofs" ) ;

      var cannotUpgradeIdxInfoCl = db.getCS( TMP_CS_UPGRADE_INDEX ).getCL( TMP_CL_CANNOT_UPGRADE_INDEX_INFO ) ;
      var sqlStr = "select * from " + TMP_CL_FULL_INVALID_CL_INFO ;
      var rc = db.exec( sqlStr ) ;
      while ( rc.next() )
      {
         var obj = rc.current().toObj() ;
         var clFullName = obj.ClFullName ;
         var csName = clFullName.split( "." )[0] ;
         var clName = clFullName.split( "." )[1] ;
         var groupName = obj.GroupName ;
         var nodeName = obj.NodeName ;
         var invalidType = obj.InvalidType ;

         var arr = nodeName.split( ":" ) ;
         var hostname = arr[0] ;
         var svcname = arr[1] ;
         var dataDB = new Sdb( hostname, svcname, USER ) ;
         var collection = dataDB.getCS( csName ).getCL( clName ) ;

         var rc1 = collection.listIndexes() ;
         while ( rc1.next() )
         {
            var indexObj = rc1.current().toObj() ;

            indexObj.IndexName = indexObj.IndexDef.name ;
            indexObj.GroupName = groupName ;
            indexObj.ClFullName = clFullName ;
            indexObj.NodeName = nodeName ;
            indexObj.InvalidType = invalidType ;
            indexObj.UpgradeIndexType = IDX_TYPE_LOCAL_IDX ;

            cannotUpgradeIdxInfoCl.insert( indexObj ) ;
         }
         dataDB.close() ;
      }

      println( "(" + step + "/" + STEP_NUM + ")End to check local collection index inofs, spent time: " +
               ( (Date.now()) - beginTime )/1000 + "s" ) ;
      step++ ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to check local collection index infos, error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

function check()
{
   checkClusterClIndexInfo() ;

   checkMainClIndexInfo() ;

   checkLocalClIndexInfo() ;
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
   var buf = "                                                                              " + s ;
   return buf.substr( buf.length - expLen ) ;
}

function writeCheckInfo( checkInfo, printRightnow )
{
   if ( 0 != checkInfo.length )
   {
      checkInfos += checkInfo ;
      infoCount++ ;
   }

   if ( printRightnow )
   {
      writeFile( CHECK_INFO_FILE_PATH, checkInfos, false ) ;
      infoCount = 0 ;
      checkInfos = "" ;
   }
   else
   {
      if ( infoCount >= PRINT_BATCH_NUM )
      {
         writeFile( CHECK_INFO_FILE_PATH, checkInfos, false ) ;
         infoCount = 0 ;
         checkInfos = "" ;
      }
   }
}

function writeUpgradeJsCode( filePath, jsCode, printRightnow )
{
   if ( 0 != jsCode.length )
   {
      jsCodes += jsCode + "\n" ;
      codeCount++ ;
   }

   if ( printRightnow )
   {
      writeFile( filePath, jsCodes, false ) ;
      codeCount = 0 ;
      jsCodes = "" ;
   }
   else
   {
      if ( codeCount >= WRITE_JS_FILE_BATCH_NUM )
      {
         writeFile( filePath, jsCodes, false ) ;
         codeCount = 0 ;
         jsCodes = "" ;
      }
   }
}

function getMaxNoNeedClFullNameLength( dataIdxCheckInfoCl, mainIdxCheckInfoCl, noNeedUpgradeIdxInfoCl )
{
   try
   {
      var rc = null ;
      var len1 = 0 ;
      var len2 = 0 ;
      var len3 = 0 ;
      var maxLength = 0 ;

      rc = dataIdxCheckInfoCl.find( { "UpgradeIndexType": "Consistent" }, { "ClFullName": { "$strlen": 1 }, "_id": 1 } ).sort( { "ClFullName": -1 } ).limit(1) ;
      len1 = rc.next() ? rc.current().toObj().ClFullName : 0 ;

      rc = mainIdxCheckInfoCl.find( { "UpgradeIndexType": "Consistent" }, { "ClFullName": { "$strlen": 1 }, "_id": 1 } ).sort( { "ClFullName": -1 } ).limit(1) ;
      len2 = rc.next() ? rc.current().toObj().ClFullName : 0 ;

      rc = noNeedUpgradeIdxInfoCl.find( {}, { "ClFullName": { "$strlen": 1 }, "_id": 1 } ).sort( { "ClFullName": -1 } ).limit(1) ;
      len3 = rc.next() ? rc.current().toObj().ClFullName : 0 ;

      maxLength = Math.max( len1, len2, len3 ) ;

      if ( maxLength < FIELD_COLLECTION.length )
      {
         maxLength = FIELD_COLLECTION.length ;
      }
      else if ( maxLength > MAX_CLFULLNAME_LENGTH )
      {
         maxLength = MAX_CLFULLNAME_LENGTH ;
      }

      return maxLength ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to get cl fullname length, error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

function getMaxNoNeedIndexNameLength( dataIdxCheckInfoCl, mainIdxCheckInfoCl, noNeedUpgradeIdxInfoCl )
{
   try
   {
      var rc = null ;
      var len1 = 0 ;
      var len2 = 0 ;
      var len3 = 0 ;
      var maxLength = 0 ;

      rc = dataIdxCheckInfoCl.find( { "UpgradeIndexType": "Consistent" }, { "IndexName": { "$strlen": 1 }, "_id": 1 } ).sort( { "IndexName": -1 } ).limit(1) ;
      len1 = rc.next() ? rc.current().toObj().IndexName : 0 ;

      rc = mainIdxCheckInfoCl.find( { "UpgradeIndexType": "Consistent" }, { "IndexName": { "$strlen": 1 }, "_id": 1 } ).sort( { "IndexName": -1 } ).limit(1) ;
      len2 = rc.next() ? rc.current().toObj().IndexName : 0 ;

      rc = noNeedUpgradeIdxInfoCl.find( {}, { "IndexName": { "$strlen": 1 }, "_id": 1 } ).sort( { "IndexName": -1 } ).limit(1) ;
      len3 = rc.next() ? rc.current().toObj().IndexName : 0 ;

      maxLength = Math.max( len1, len2, len3 ) ;

      if ( maxLength < FIELD_INDEXNAME.length )
      {
         maxLength = FIELD_INDEXNAME.length ;
      }
      else if ( maxLength > FIELD_INDEXNAME )
      {
         maxLength = FIELD_INDEXNAME ;
      }

      return maxLength ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to get index name length, error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

function writeNoNeedUpgradeIdxReport( noNeedIdxCount, dataIdxCheckInfoCl, mainIdxCheckInfoCl, noNeedUpgradeIdxInfoCl )
{
   try
   {
      var rc = null ;
      var id = 1 ;
      var beginTime = Date.now() ;
      var printProgressCount = _PRINT_PROGRESS_COUNT ;
      var tmpNum = noNeedIdxCount/printProgressCount ;
      tmpNum = tmpNum > 1 ? tmpNum : 1 ;
      var aProgressCollectClNum = tmpNum ;

      println( "(" + step + "/" + STEP_NUM + ")Begin to write no need upgrade index infos" ) ;
      println( "(" + step + "/" + STEP_NUM + ")No Need Upgrade Indexes Count: " + noNeedIdxCount ) ;
      // 每完成四十分之一打印一个 #
      println( "Writing\n0% ______________ 50% ______________ 100%" ) ;

      var maxClFullNameLength = getMaxNoNeedClFullNameLength( dataIdxCheckInfoCl,
                                                              mainIdxCheckInfoCl,
                                                              noNeedUpgradeIdxInfoCl ) ;
      var maxIndexNameLength = getMaxNoNeedIndexNameLength( dataIdxCheckInfoCl,
                                                            mainIdxCheckInfoCl,
                                                            noNeedUpgradeIdxInfoCl ) ;

      var idLen = ( noNeedIdxCount + "" ).length ;
      idLen = Math.max( idLen, FIELD_ID.length + 1 ) ;

      writeCheckInfo( "===================== Check Result ( No Need to Upgrade ) ======================\n" +
                      pad( FIELD_ID, idLen ) + " " +
                      pad( FIELD_COLLECTION, maxClFullNameLength ) + " " +
                      pad( FIELD_INDEXNAME, maxIndexNameLength ) + " " +
                      pad( FIELD_INDEXTYPE, NO_NEED_MAX_INDEX_TYPE_LENGTH ) + "\n", false ) ;

      rc = dataIdxCheckInfoCl.find( { "UpgradeIndexType": IDX_TYPE_CONSISTENT } ).sort( { "ClFullName": 1 } ) ;
      while ( rc.next() )
      {
         var obj = rc.current().toObj() ;
         if (null == obj.ClFullName || null == obj.IndexName) { continue ; }
         writeCheckInfo( pad( id, idLen ) + " " +
                         pad( obj.ClFullName, maxClFullNameLength ) + " " +
                         pad( obj.IndexName, maxIndexNameLength ) + " " +
                         pad( IDX_TYPE_CONSISTENT, NO_NEED_MAX_INDEX_TYPE_LENGTH ) + "\n", false ) ;
         id++ ;
         while ( id >= tmpNum && 0 != printProgressCount )
         {
            print( "#" ) ;
            tmpNum += aProgressCollectClNum ;
            printProgressCount-- ;
         }
      }

      rc = mainIdxCheckInfoCl.find( { "UpgradeIndexType": IDX_TYPE_CONSISTENT } ).sort( { "ClFullName": 1 } ) ;
      while ( rc.next() )
      {
         var obj = rc.current().toObj() ;
         if (null == obj.ClFullName || null == obj.IndexName) { continue ; }
         writeCheckInfo( pad( id, idLen ) + " " +
                         pad( obj.ClFullName, maxClFullNameLength ) + " " +
                         pad( obj.IndexName, maxIndexNameLength ) + " " +
                         pad( IDX_TYPE_CONSISTENT, NO_NEED_MAX_INDEX_TYPE_LENGTH ) + "\n", false ) ;
         id++ ;
         while ( id >= tmpNum && 0 != printProgressCount )
         {
            print( "#" ) ;
            tmpNum += aProgressCollectClNum ;
            printProgressCount-- ;
         }
      }

      rc = noNeedUpgradeIdxInfoCl.find().sort( { "ClFullName": 1 } ) ;
      while ( rc.next() )
      {
         var obj = rc.current().toObj() ;
         if (null == obj.ClFullName || null == obj.IndexName) { continue ; }
         writeCheckInfo( pad( id, idLen ) + " " +
                         pad( obj.ClFullName, maxClFullNameLength ) + " " +
                         pad( obj.IndexName, maxIndexNameLength ) + " " +
                         pad( IDX_TYPE_STANDALONE, NO_NEED_MAX_INDEX_TYPE_LENGTH ) + "\n", false ) ;
         id++ ;
         while ( id >= tmpNum && 0 != printProgressCount )
         {
            print( "#" ) ;
            tmpNum += aProgressCollectClNum ;
            printProgressCount-- ;
         }
      }

      writeCheckInfo( "\n", true ) ;

      for ( var i = 0 ; i < printProgressCount ; i++ )
      {
         print( "#" ) ;
      }
      print( "\n" ) ;

      println( "(" + step + "/" + STEP_NUM + ")End to write no need upgrade index infos, spent time: " +
               ( (Date.now()) - beginTime )/1000 + "s" ) ;
      step++ ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to write no need upgrade index infos, error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
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

   if ( attr.length > 0 )
   {
      return attr.substr( 1 ) ;
   }
   else
   {
      return "-" ;
   }
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

   return attr ;
}

function generateCreateIdxJsCodeWithoutSpentTime( clFullName, indexName, idxDef )
{
   try
   {
      var code = "var collection = db.getCS( '" +
                 clFullName.split('.')[0] + "' ).getCL( '" +
                 clFullName.split('.')[1] + "' ) ;\n" ;

      if ( indexName == "$id" )
      {
         if ( supportOnlyUpgradeMeta )
         {
            code += "collection.createIdIndex( { 'OnlyUpgradeMeta': true } ) ;\n" ;
         }
         else
         {
            code += "collection.createIdIndex() ;\n" ;
         }
         code += ( "println('Create index successfully[ ClFullName: " +
                 clFullName + ", IndexName: " + indexName + " ]' ) ;\n" ) ;
      }
      else if ( indexName == "$shard" )
      {
         code += "enableSharding( '" + clFullName + "', '" + indexName + "', collection, { 'ShardingKey': " + JSON.stringify( idxDef.key ) + " } ) ;\n" ;
      }
      else
      {
         code += "createIndex( '" + clFullName + "', collection, '" + indexName +
                  "', " + JSON.stringify( idxDef.key ) + ", " +
                  JSON.stringify( getIdxAttr( idxDef ) ) + ", " + supportOnlyUpgradeMeta + " ) ;\n" ;
      }

      return code ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to generate createIndex js code without spent time, error Stack:\n" +
                  e.stack ) ;
      }
      throw e ;
   }
}

function generateCreateIdxJsCodeWithSpentTime( clFullName, indexName, idxDef )
{
   try
   {
      var code = "var beginTime = Date.now() ;\n" ;
      code += ( "println('Begin to create index [ ClFullName: " +
                clFullName + ", IndexName: " + indexName + " ]' ) ;\n" ) ;
      code += ( "var collection = db.getCS( '" +
              clFullName.split('.')[0] + "' ).getCL( '" +
              clFullName.split('.')[1] + "' ) ;\n" ) ;

      if ( indexName == "$id" )
      {
         code += "collection.createIdIndex() ;\n" ;
      }
      else if ( indexName == "$shard" )
      {
         code += "collection.enableSharding( { 'ShardingKey': " + JSON.stringify( idxDef.key ) + " } ) ;\n" ;
      }
      else
      {
         code += "collection.createIndex( '" + indexName +
                 "', " + JSON.stringify( idxDef.key ) + ", " +
                 JSON.stringify( getIdxAttr( idxDef ) ) + " ) ;\n" ;
      }

      code += ( "println('Create index successfully, spent time: ' + ( (Date.now()) - beginTime )/1000 + 's' ) ;\n" ) ;

      return code ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to generate createIndex js code with spent time, error Stack:\n" +
                  e.stack ) ;
      }
      throw e ;
   }
}

function generateDropIdxJsCode( clFullName, indexName )
{
   try
   {
      var code = "var beginTime = Date.now() ;\n" ;
      code += ( "println('Begin to drop index [ ClFullName: " +
                clFullName + ", IndexName: " + indexName + " ]' ) ;\n" ) ;
      code += ( "var collection = db.getCS( '" +
              clFullName.split('.')[0] + "' ).getCL( '" +
              clFullName.split('.')[1] + "' ) ;\n" ) ;

      if ( indexName == "$id" )
      {
         code += "collection.dropIdIndex() ;\n" ;
      }
      else if ( indexName == "$shard" )
      {
         // do nothing
      }
      else
      {
         code += "collection.dropIndex( '" + indexName + "' ) ;\n" ;
      }

      code += ( "println('Drop index successfully, spent time: ' + ( (Date.now()) - beginTime )/1000 + 's' ) ;\n" ) ;

      return code ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to generate dropIndex js code, error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

function generateUpgradeIdxJsCode( indexInfoObj )
{
   return generateCreateIdxJsCodeWithoutSpentTime( indexInfoObj.ClFullName, indexInfoObj.IndexName, indexInfoObj.IndexDef ) ;
}

function generateSdbConnStr( hostname, svcname, username, passwd, cipherFile, token )
{
   var ciperUserStr = "" ;
   var sdbConnStr = "" ;

   if ( cipherFile == "" )
   {
      sdbConnStr = "var db = new Sdb( '" + hostname + "', '" + svcname + "', '" + username + "', '" + passwd + "' ) ;\n"
   }
   else
   {
      var ciperUserStr = "" ;
      if ( cipherFile == "~/sequoiadb/passwd" )
      {
         ciperUserStr = "var USER = new CipherUser( '" + username + "' ).token( '" + token + "' ) ;\n" ;
      }
      else
      {
         ciperUserStr = "var USER = new CipherUser( '" + username + "' ).token( '" + token + "' ).cipherFile( '" + cipherFile + "' ) ;\n" ;
      }
      sdbConnStr = ciperUserStr + "var db = new Sdb( '" + hostname + "', '" + svcname + "', USER ) ;\n" ;
   }

   return sdbConnStr ;
}

function writeFile( filePath, content, needTruncate )
{
   try
   {
      var file = new File( filePath, 0644, SDB_FILE_READWRITE|SDB_FILE_CREATE ) ;

      if ( needTruncate )
      {
         file.truncate() ;
      }

      file.seek( 0, "e" ) ;
      file.write( content ) ;
      file.close() ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to write file['" + filePath + "'], error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

function getMaxCanClFullNameLength( dataIdxCheckInfoCl, mainIdxCheckInfoCl )
{
   try
   {
      var rc = null ;
      var len1 = 0 ;
      var len2 = 0 ;
      var maxLength = 0 ;

      rc = dataIdxCheckInfoCl.find( { "UpgradeIndexType": { "$isnull": 1 } }, { "ClFullName": { "$strlen": 1 }, "_id": 1 } ).sort( { "ClFullName": -1 } ).limit(1) ;
      len1 = rc.next() ? rc.current().toObj().ClFullName : 0 ;

      rc = mainIdxCheckInfoCl.find( { "UpgradeIndexType": { "$isnull": 1 } }, { "ClFullName": { "$strlen": 1 }, "_id": 1 } ).sort( { "ClFullName": -1 } ).limit(1) ;
      len2 = rc.next() ? rc.current().toObj().ClFullName : 0 ;

      maxLength = Math.max( len1, len2 ) ;

      if ( maxLength < FIELD_COLLECTION.length )
      {
         maxLength = FIELD_COLLECTION.length ;
      }
      else if ( maxLength > MAX_CLFULLNAME_LENGTH )
      {
         maxLength = MAX_CLFULLNAME_LENGTH ;
      }

      return maxLength ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to get cl fullname length, error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

function getMaxCanIndexNameLength( dataIdxCheckInfoCl, mainIdxCheckInfoCl )
{
   try
   {
      var rc = null ;
      var len1 = 0 ;
      var len2 = 0 ;
      var maxLength = 0 ;

      rc = dataIdxCheckInfoCl.find( { "UpgradeIndexType": { "$isnull": 1 } }, { "IndexName": { "$strlen": 1 }, "_id": 1 } ).sort( { "IndexName": -1 } ).limit(1) ;
      len1 = rc.next() ? rc.current().toObj().IndexName : 0 ;

      rc = mainIdxCheckInfoCl.find( { "UpgradeIndexType": { "$isnull": 1 } }, { "IndexName": { "$strlen": 1 }, "_id": 1 } ).sort( { "IndexName": -1 } ).limit(1) ;
      len2 = rc.next() ? rc.current().toObj().IndexName : 0 ;

      maxLength = Math.max( len1, len2 ) ;

      if ( maxLength < FIELD_INDEXNAME.length )
      {
         maxLength = FIELD_INDEXNAME.length ;
      }
      else if ( maxLength > MAX_INDEX_NAME_LENGTH )
      {
         maxLength = MAX_INDEX_NAME_LENGTH ;
      }

      return maxLength ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to get index name length, error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

function writeCanUpgradeIdxReport( canUpgradeIdxCount, dataIdxCheckInfoCl, mainIdxCheckInfoCl )
{
   try
   {
      var rc = null ;
      var id = 1 ;
      var beginTime = Date.now() ;
      var printProgressCount = _PRINT_PROGRESS_COUNT ;
      var printClTmpNum = canUpgradeIdxCount/printProgressCount ;
      printClTmpNum = printClTmpNum > 1 ? printClTmpNum : 1 ;
      var aProgressCollectClNum = printClTmpNum ;

      println( "(" + step + "/" + STEP_NUM + ")Begin to write can upgrade index infos" ) ;
      println( "(" + step + "/" + STEP_NUM + ")Can Upgrade Indexes Count: " + canUpgradeIdxCount ) ;
      // 每完成四十分之一打印一个 #
      println( "Writing\n0% ______________ 50% ______________ 100%" ) ;

      var idLen = ( canUpgradeIdxCount + "" ).length ;
      idLen = Math.max( idLen, FIELD_ID.length + 1 ) ;

      var maxClFullNameLength = getMaxCanClFullNameLength( dataIdxCheckInfoCl, mainIdxCheckInfoCl ) ;
      var maxIndexNameLength = getMaxCanIndexNameLength( dataIdxCheckInfoCl, mainIdxCheckInfoCl ) ;

      writeCheckInfo( "===================== Check Result ( Can be Upgraded ) =========================\n" +
                      pad( FIELD_ID, idLen ) + " " +
                      pad( FIELD_COLLECTION, maxClFullNameLength ) + " " +
                      pad( FIELD_INDEXNAME, maxIndexNameLength ) + " " +
                      pad( FIELD_INDEXATTR, MAX_INDEX_ATTR_DESC_LENGTH ) + " " +
                      FIELD_INDEXKEY + "\n", false ) ;

      rc = dataIdxCheckInfoCl.find( { "UpgradeIndexType": { "$isnull": 1 } } ).sort( { "ClFullName": 1 } ) ;
      while ( rc.next() )
      {
         var obj = rc.current().toObj() ;
         writeCheckInfo( pad( id, idLen ) + " " +
                         pad( obj.ClFullName, maxClFullNameLength ) + " " +
                         pad( obj.IndexName, maxIndexNameLength ) + " " +
                         pad( getIdxAttrDesc( obj.IndexDef ), MAX_INDEX_ATTR_DESC_LENGTH ) + " " +
                         JSON.stringify( obj.IndexDef.key ) + "\n", false ) ;
         id++ ;
         if ( id >= printClTmpNum && 0 != printProgressCount )
         {
            print( "#" ) ;
            printClTmpNum += aProgressCollectClNum ;
            printProgressCount-- ;
         }
      }

      rc = mainIdxCheckInfoCl.find( { "UpgradeIndexType": { "$isnull": 1 } } ).sort( { "ClFullName": 1 } ) ;
      while ( rc.next() )
      {
         var obj = rc.current().toObj() ;
         writeCheckInfo( pad( id, idLen ) + " " +
                         pad( obj.ClFullName, maxClFullNameLength ) + " " +
                         pad( obj.IndexName, maxIndexNameLength ) + " " +
                         pad( getIdxAttrDesc( obj.IndexDef ), MAX_INDEX_ATTR_DESC_LENGTH ) + " " +
                         JSON.stringify( obj.IndexDef.key ) + "\n", false ) ;
         id++ ;
         if ( id >= printClTmpNum && 0 != printProgressCount )
         {
            print( "#" ) ;
            printClTmpNum += aProgressCollectClNum ;
            printProgressCount-- ;
         }
      }

      writeCheckInfo( "\n", true ) ;

      for ( var i = 0 ; i < printProgressCount ; i++ )
      {
         print( "#" ) ;
      }
      print( "\n" ) ;

      println( "(" + step + "/" + STEP_NUM + ")End to write can upgrade index infos, spent time: " +
               ( (Date.now()) - beginTime )/1000 + "s" ) ;
      step++ ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to write can upgrade index infos, error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

function generateMissIdxDetailInfo( id, indexInfoObj, missIdxDetail )
{
   try
   {
      var detailStr = "" ;
      var clGroups = null ;
      var cataClusterClInfoCl = db.getCS( TMP_CS_UPGRADE_INDEX ).getCL( TMP_CL_CATA_CLUSTER_CL_INFO ) ;
      var missGroups = indexInfoObj.Groups ;

      var rc = cataClusterClInfoCl.find( { "ClFullName": indexInfoObj.ClFullName }, { "Groups": 1 } ) ;
      clGroups = rc.current().toObj().Groups ;

      for ( i in clGroups )
      {
         var clGroup = clGroups[i].GroupName ;
         var clNodeList = clGroups[i].NodeList ;

         for ( j in clNodeList )
         {
            var missStr = "" ;
            var hasFound = false ;
            var node = clNodeList[j].HostName + ":" + clNodeList[j].NodeName ;

            for ( k in missGroups )
            {
               var missGroup = missGroups[k].GroupName ;

               if ( missGroup == clGroup )
               {
                  var missNodeList = missGroups[k].NodeInfos ;

                  for ( l in missNodeList )
                  {
                     var missNode = missNodeList[l].NodeName ;

                     if ( node == missNode )
                     {
                        hasFound = true ;
                        break ;
                     }
                  }
                  if ( hasFound )
                  {
                     break ;
                  }
               }
            }

            if ( hasFound )
            {
               missStr = "Y" ;
            }
            else
            {
               missStr = "N" ;
            }

            detailStr += "  " + pad( clGroup, MAX_GROUP_NAME_LENGTH ) + " " +
                         pad( node, MAX_NODE_NAME_LENGTH ) + " " +
                         pad( missStr, IDX_TYPE_MISSING.length ) + "\n" ;
         }
      }

      missIdxDetail.push( { "ID": id, "Detail": detailStr } ) ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to generate miss index detail info, error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

function generateConflictIdxDetailInfo( id, indexInfoObj, conflictIdxDetail,
                                        maxIndexNameLength, maxConflictIndexKeyLength )
{
   try
   {
      var detailStr = "" ;

      if ( undefined != indexInfoObj.IndexNameCount && indexInfoObj.IndexNameCount > 1 )
      {
         // same index def, diff index name
         for ( i in indexInfoObj.IndexNames )
         {
            var indexName = indexInfoObj.IndexNames[i] ;
            var firstFoundIdxInfo = true ;
            var firstGroup = true ;

            detailStr += "  " + pad( indexName, maxIndexNameLength ) ;

            for ( j in indexInfoObj.Groups )
            {
               var group = indexInfoObj.Groups[j] ;
               var groupName = group.GroupName ;
               var nodeInfos = group.NodeInfos ;
               var nodeNameStr = "" ;

               for ( k in nodeInfos )
               {
                  var nodeInfo = nodeInfos[k] ;
                  var idxName = nodeInfo.IndexName ;

                  if ( idxName == indexName )
                  {
                     if ( firstFoundIdxInfo )
                     {
                        detailStr += " " +
                                     pad( JSON.stringify( nodeInfo.IndexDef.key ), maxConflictIndexKeyLength ) + " " +
                                     pad( getIdxAttrDesc( indexInfoObj.IndexDef ), MAX_INDEX_ATTR_DESC_LENGTH ) + " " ;
                        firstFoundIdxInfo = false ;
                     }
                     nodeNameStr += nodeInfo.NodeName + "," ;
                  }
               }
               nodeNameStr = nodeNameStr.slice( 0, -1 ) ;

               if ( firstGroup )
               {
                  detailStr += pad( groupName, MAX_GROUP_NAME_LENGTH ) + "  " + nodeNameStr + "\n" ;
               }
               else
               {
                  detailStr += "  " + pad( "" ) + " " + pad( "" ) + " " + pad( "" ) + " " +
                               pad( groupName, MAX_GROUP_NAME_LENGTH ) + "  " + nodeNameStr + "\n" ;
               }
            }
         }
      }
      else if ( undefined != indexInfoObj.IndexDefCount && indexInfoObj.IndexDefCount > 1 )
      {
         // diff index def, same index name
         var indexName = indexInfoObj.IndexName ;

         for ( i in indexInfoObj.IndexDefs )
         {
            var indexDef = indexInfoObj.IndexDefs[i] ;
            var firstFoundIdxInfo = true ;
            var firstGroup = true ;

            detailStr += "  " + pad( indexName, maxIndexNameLength ) ;

            for ( j in indexInfoObj.Groups )
            {
               var group = indexInfoObj.Groups[j] ;
               var groupName = group.GroupName ;
               var nodeInfos = group.NodeInfos ;
               var nodeNameStr = "" ;

               for ( k in nodeInfos )
               {
                  var nodeInfo = nodeInfos[k] ;
                  var idxDef = nodeInfo.IndexDef ;

                  if ( JSON.stringify( idxDef ) == JSON.stringify( indexDef ) )
                  {
                     if ( firstFoundIdxInfo )
                     {
                        detailStr += " " +
                                     pad( JSON.stringify( indexDef.key ), maxConflictIndexKeyLength ) + " " +
                                     pad( getIdxAttrDesc( indexDef ), MAX_INDEX_ATTR_DESC_LENGTH ) + " " ;
                        firstFoundIdxInfo = false ;
                     }
                     nodeNameStr += nodeInfo.NodeName + "," ;
                  }
               }
               nodeNameStr = nodeNameStr.slice( 0, -1 ) ;

               if ( firstGroup )
               {
                  detailStr += pad( groupName, MAX_GROUP_NAME_LENGTH ) + "  " + nodeNameStr + "\n" ;
               }
               else
               {
                  detailStr += "  " + pad( "" ) + " " + pad( "" ) + " " + pad( "" ) + " " +
                               pad( groupName, MAX_GROUP_NAME_LENGTH ) + "  " + nodeNameStr + "\n" ;
               }
            }
         }
      }

      conflictIdxDetail.push( { "ID": id, "Detail": detailStr } ) ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to generate conflict index detail info, error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

function generateInvalShardIdIdxDetailInfo( id, indexInfoObj, invalidShardIdxDetail, maxClFullNameLength )
{
   try
   {
      var detailStr = "" ;

      for ( i in indexInfoObj.DataGroups )
      {
         var group = indexInfoObj.DataGroups[i] ;
         var nodeInfos = group.NodeInfos ;
         for ( j in nodeInfos )
         {
            var nodeName = nodeInfos[j].NodeName ;
            detailStr += "  " + pad( indexInfoObj.ClFullName, maxClFullNameLength ) + " " + nodeName + "\n" ;
         }
      }

      invalidShardIdxDetail.push( { "ID": id, "Detail": detailStr } ) ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to generate invalid $shard index detail info, error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

function generateInvalidIdIdxDetailInfo( id, indexInfoObj, invalidIdIdxDetail, maxClFullNameLength )
{
   try
   {
      var detailStr = "" ;

      for ( i in indexInfoObj.DataGroups )
      {
         var group = indexInfoObj.DataGroups[i] ;
         var nodeInfos = group.NodeInfos ;
         for ( j in nodeInfos )
         {
            var nodeName = nodeInfos[j].NodeName ;
            detailStr += "  " + pad( indexInfoObj.ClFullName, maxClFullNameLength ) + " " + nodeName + "\n" ;
         }
      }

      invalidIdIdxDetail.push( { "ID": id, "Detail": detailStr } ) ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to generate invalid $id index detail info, error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

function generateLocalClIdxDetailInfo( id, indexInfoObj, localclIdxDetail, maxClFullNameLength )
{
   try
   {
      var detailStr = "" ;

      detailStr += "  " + pad( indexInfoObj.ClFullName, maxClFullNameLength ) + " " + indexInfoObj.NodeName + "\n" ;

      localclIdxDetail.push( { "ID": id, "Detail": detailStr, "InvalidType": indexInfoObj.InvalidType } ) ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to generate local cl index detail info, error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

/*

  ---------- Index ( ID: 19 ) InvalidUniqueIndex -----------
                                    Collection        IndexName           IndexKey MainAndSubCLShardingKeys
    csName_test.sharding_without_unique_maincl all_unique_idx_1  {"all_field_1":1} { "main_shard": 1 }
                                                                                   { "no1": 1 }
                                                                                   { "no2": 1 }

*/
function generateInvalidUniqueIdxDetailInfo( id, indexInfoObj, invalidUniqueIdxDetail, maxClFullNameLength,
                                             maxIndexNameLength, maxInvalidUniqueIndexKeyLength )
{
   try
   {
      var detailStr = "" ;
      var tmpLength = 0 ;

      detailStr += "  " +
                   pad( indexInfoObj.ClFullName, maxClFullNameLength ) + " " +
                   pad( indexInfoObj.IndexName, maxIndexNameLength ) + " " +
                   pad( JSON.stringify( indexInfoObj.IndexDef.key ), maxInvalidUniqueIndexKeyLength ) + " " ;

      tmpLength = detailStr.length ;
      var emptyStr = Array( tmpLength + 1 ).join( ' ' ) ;

      detailStr += JSON.stringify( indexInfoObj.ShardingKey ) + "\n" ;

      for ( i in indexInfoObj.SubClShardingKeys )
      {
         detailStr += emptyStr + JSON.stringify( indexInfoObj.SubClShardingKeys[i] ) + "\n" ;
      }

      invalidUniqueIdxDetail.push( { "ID": id, "Detail": detailStr, "InvalidType": indexInfoObj.InvalidType } ) ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to generate invalid unique index detail info, error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

function generateMissIdxJsCode( indexInfoObj )
{
   var jsCode = "// Deal with Miss Indexs\n" ;

   if ( IGNORE_STRATEGY == DEALWITH_MISS_INDEX )
   {
      return jsCode ;
   }

   try
   {
      var groups = indexInfoObj.Groups ;

      if ( DROP_STRATEGY == DEALWITH_MISS_INDEX )
      {
         jsCode += "/*\n" ;
         // drop idx
         for ( i in groups )
         {
            var group = groups[i] ;
            var nodeInfos = group.NodeInfos ;
            for ( j in nodeInfos )
            {
               var nodeName = nodeInfos[j].NodeName ;
               var hostname = nodeName.split( ":" )[0] ;
               var svcname = nodeName.split( ":" )[1] ;

               jsCode += generateSdbConnStr( hostname, svcname, USERNAME, PASSWD, CIPHER_FILE, TOKEN ) ;
               jsCode += generateDropIdxJsCode( indexInfoObj.ClFullName, indexInfoObj.IndexName ) ;
               jsCode += "db.close() ;\n\n" ;
            }
         }
         jsCode += "*/\n" ;
      }
      else if ( RE_CREATE_STRATEGY == DEALWITH_MISS_INDEX )
      {
         // recreate idx
         jsCode += generateSdbConnStr( HOSTNAME, SVCNAME, USERNAME, PASSWD, CIPHER_FILE, TOKEN ) ;
         jsCode += generateCreateIdxJsCodeWithSpentTime( indexInfoObj.ClFullName, indexInfoObj.IndexName, indexInfoObj.IndexDef ) ;
         jsCode += "db.close() ;\n" ;
      }
      else
      {
         // ignore ;
      }

      return jsCode ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to generate drop miss index js code, error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

function generateConflictIdxJsCode( indexInfoObj )
{
   var jsCode = "// Deal with Conflict Indexs\n" ;

   if ( IGNORE_STRATEGY == DEALWITH_CONFLICT_INDEX )
   {
      return jsCode ;
   }

   try
   {
      var groups = indexInfoObj.Groups ;
      var idxDef = null ;
      var idxName = null ;

      jsCode += "/*\n" ;

      for ( i in groups )
      {
         var group = groups[i] ;
         var nodeInfos = group.NodeInfos ;
         for ( j in nodeInfos )
         {
            var nodeName = nodeInfos[j].NodeName ;
            var hostname = nodeName.split( ":" )[0] ;
            var svcname = nodeName.split( ":" )[1] ;
            idxName = nodeInfos[j].IndexName ;
            idxDef = nodeInfos[j].IndexDef ;

            jsCode += generateSdbConnStr( hostname, svcname, USERNAME, PASSWD, CIPHER_FILE, TOKEN ) ;
            jsCode += generateDropIdxJsCode( indexInfoObj.ClFullName, idxName ) ;
            jsCode += "db.close() ;\n\n" ;
         }
      }

      if ( RE_CREATE_STRATEGY == DEALWITH_CONFLICT_INDEX )
      {
         // recreate idx
         jsCode += generateSdbConnStr( HOSTNAME, SVCNAME, USERNAME, PASSWD, CIPHER_FILE, TOKEN ) ;
         jsCode += generateCreateIdxJsCodeWithSpentTime( indexInfoObj.ClFullName, idxName, idxDef ) ;
         jsCode += "db.close() ;\n\n" ;
      }
      else
      {
         // ignore ;
      }
      jsCode += "*/\n" ;

      return jsCode ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to generate drop conflict index js code, error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

function generateInvalidIdIdxJsCode( indexInfoObj )
{
   var jsCode = "// Deal with Invalid $id Indexs\n" ;

   try
   {
      jsCode += "/*\n" ;
      jsCode += generateSdbConnStr( HOSTNAME, SVCNAME, USERNAME, PASSWD, CIPHER_FILE, TOKEN ) ;
      jsCode += generateDropIdxJsCode( indexInfoObj.ClFullName, indexInfoObj.IndexName ) ;
      jsCode += "db.close() ;\n\n" ;
      jsCode += "*/\n" ;
      return jsCode ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to generate drop invalid $id index js code, error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

function writeAndgenerateLocalClJsCode()
{
   var jsCode = "// Deal with Local collections\n" ;

   if ( IGNORE_STRATEGY == DEALWITH_LOCAL_CL )
   {
      return ;
   }

   try
   {
      var sqlStr = "select * from " + TMP_CL_FULL_INVALID_CL_INFO ;
      var count = 0 ;
      var rc = db.exec( sqlStr ) ;
      while ( rc.next() )
      {
         var obj = rc.current().toObj() ;

         var clFullName = obj.ClFullName ;
         var nodeName = obj.NodeName ;

         var hostname = nodeName.split( ":" )[0] ;
         var svcname = nodeName.split( ":" )[1] ;

         var csName = clFullName.split( "." )[0] ;
         var clName = clFullName.split( "." )[1] ;

         jsCode += "/*\n" ;
         jsCode += generateSdbConnStr( hostname, svcname, USERNAME, PASSWD, CIPHER_FILE, TOKEN ) ;
         jsCode += "var beginTime = Date.now() ;\n" ;
         jsCode += ( "println( 'Begin to drop invalid collection [ ClFullName: " + clFullName + " ]' ) ;\n" ) ;
         jsCode += "db." + csName + ".dropCL( '" + clName + "' ) ;\n" ;
         jsCode += ( "println('Drop invalid collection successfully, spent time: ' + ( (Date.now()) - beginTime )/1000 + 's' ) ;\n" ) ;
         jsCode += "db.close() ;\n\n" ;
         jsCode += "*/\n" ;

         count++ ;
      }

      if ( count > 0 )
      {
         writeFile( LOCAL_CL_JS_FILE, jsCode, false ) ;
      }
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to generate drop local cl js code, error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

function getMaxCannotClFullNameLength( cannotUpgradeIdxInfoCl )
{
   try
   {
      var rc = null ;
      var maxLength = 0 ;

      rc = cannotUpgradeIdxInfoCl.find( {}, { "ClFullName": { "$strlen": 1 }, "_id": 1 } ).sort( { "ClFullName": -1 } ).limit(1) ;
      maxLength = rc.next() ? rc.current().toObj().ClFullName : 0 ;

      if ( maxLength < FIELD_COLLECTION.length )
      {
         maxLength = FIELD_COLLECTION.length ;
      }
      else if ( maxLength > MAX_CLFULLNAME_LENGTH )
      {
         maxLength = MAX_CLFULLNAME_LENGTH ;
      }

      return maxLength ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to get cl fullname length, error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

function getMaxCannotIndexNameLength( cannotUpgradeIdxInfoCl )
{
   try
   {
      var rc = null ;
      var len1 = 0 ;
      var len2 = 0 ;
      var maxLength = 0 ;

      rc = cannotUpgradeIdxInfoCl.find( { "IndexNames": { "$isnull": 0 } }, { "IndexNames": { "$strlen": 1 }, "_id": 1 } ).sort( { "IndexNames": -1 } ).limit(1) ;
      while ( rc.next() )
      {
         var indexNames = rc.current().toObj().IndexNames ;
         for ( i in indexNames )
         {
            if ( len1 < indexNames[i] )
            {
               len1 = indexNames[i] ;
            }
         }
      }

      rc = cannotUpgradeIdxInfoCl.find( { "IndexNames": { "$isnull": 1 } }, { "IndexName": { "$strlen": 1 }, "_id": 1 } ).sort( { "IndexName": -1 } ).limit(1) ;
      len2 = rc.next() ? rc.current().toObj().IndexName : 0 ;

      maxLength = Math.max( len1, len2 ) ;

      if ( maxLength < FIELD_INDEXNAME.length )
      {
         maxLength = FIELD_INDEXNAME.length ;
      }
      else if ( maxLength > MAX_INDEX_NAME_LENGTH )
      {
         maxLength = MAX_INDEX_NAME_LENGTH ;
      }

      return maxLength ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to get index name length, error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

function getMaxCannotUpgradeIndexTypeLength( cannotUpgradeIdxInfoCl )
{
   try
   {
      var rc = null ;
      var maxLength = 0 ;

      rc = cannotUpgradeIdxInfoCl.find( {}, { "UpgradeIndexType": { "$strlen": 1 }, "_id": 1 } ).sort( { "UpgradeIndexType": -1 } ).limit(1) ;
      maxLength = rc.next() ? rc.current().toObj().UpgradeIndexType : 0 ;

      if ( maxLength > MAX_INDEX_TYPE_LENGTH )
      {
         maxLength = MAX_INDEX_TYPE_LENGTH ;
      }

      return maxLength ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to get cl fullname length, error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

function getMaxConflictIndexKeyLength( cannotUpgradeIdxInfoCl )
{
   try
   {
      var rc = null ;
      var maxLength = 0 ;

      rc = cannotUpgradeIdxInfoCl.find( { "UpgradeIndexType": IDX_TYPE_CONFLICT } ) ;
      while ( rc.next() )
      {
         var obj = rc.current().toObj() ;

         if ( undefined != obj.IndexDef )
         {
            var key = JSON.stringify( obj.IndexDef.key ) ;
            if ( maxLength < key.length )
            {
               maxLength = key.length ;
            }
         }
         else if( undefined != obj.IndexDefs )
         {
            for ( i in obj.IndexDefs )
            {
               var key = JSON.stringify( obj.IndexDefs[i].key ) ;
               if ( maxLength < key.length )
               {
                  maxLength = key.length ;
               }
            }
         }
      }

      if ( maxLength < FIELD_INDEXKEY.length )
      {
         maxLength = FIELD_INDEXKEY.length ;
      }
      else if ( maxLength > MAX_INDEX_KEY_LENGTH )
      {
         maxLength = MAX_INDEX_KEY_LENGTH ;
      }

      return maxLength ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to get max conflict index key length, error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

function getMaxInvalidUniqueIndexKeyLength( cannotUpgradeIdxInfoCl )
{
   try
   {
      var rc = null ;
      var maxLength = 0 ;

      rc = cannotUpgradeIdxInfoCl.find( { "UpgradeIndexType": IDX_TYPE_INVALID_UNI } ) ;
      while ( rc.next() )
      {
         var obj = rc.current().toObj() ;
         var key = JSON.stringify( obj.IndexDef.key ) ;
         if ( maxLength < key.length )
         {
            maxLength = key.length ;
         }
      }

      if ( maxLength < FIELD_INDEXKEY.length )
      {
         maxLength = FIELD_INDEXKEY.length ;
      }
      else if ( maxLength > MAX_INDEX_KEY_LENGTH )
      {
         maxLength = MAX_INDEX_KEY_LENGTH ;
      }

      return maxLength ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to get max invalid unique index key length, error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

function writeCannotUpgradeIdxReport( cannotUpgradeIdxCount, cannotUpgradeIdxInfoCl )
{
   try
   {
      var rc = null ;
      var id = 1 ;
      var beginTime = Date.now() ;
      var missIdxDetail = [] ;
      var conflictIdxDetail = [] ;
      var invalidIdIdxDetail = [] ;
      var invalidShardIdxDetail = [] ;
      var localclIdxDetail = [] ;
      var invalidUniqueIdxDetail = [] ;

      println( "(" + step + "/" + STEP_NUM + ")Begin to write cannot upgrade index infos" ) ;
      println( "(" + step + "/" + STEP_NUM + ")Cannot Indexes Count: " + cannotUpgradeIdxCount ) ;

      var idLen = ( cannotUpgradeIdxCount + "" ).length ;
      idLen = Math.max( idLen, FIELD_ID.length + 1 ) ;

      var maxClFullNameLength = getMaxCannotClFullNameLength( cannotUpgradeIdxInfoCl ) ;

      var maxIndexNameLength = getMaxCannotIndexNameLength( cannotUpgradeIdxInfoCl ) ;

      var maxIndexTypeLength = getMaxCannotUpgradeIndexTypeLength( cannotUpgradeIdxInfoCl ) ;

      var maxConflictIndexKeyLength = getMaxConflictIndexKeyLength( cannotUpgradeIdxInfoCl ) ;

      var maxInvalidUniqueIndexKeyLength = getMaxInvalidUniqueIndexKeyLength( cannotUpgradeIdxInfoCl ) ;

      writeCheckInfo( "===================== Check Result ( Cannot be Upgraded ) ======================\n" +
                      pad( FIELD_ID, idLen ) + " " +
                      pad( FIELD_REASON, maxIndexTypeLength ) + " " +
                      pad( FIELD_COLLECTION, maxClFullNameLength )   + " " +
                      pad( FIELD_INDEXNAME, maxIndexNameLength ) + " " +
                      pad( FIELD_INDEXATTR, MAX_INDEX_ATTR_DESC_LENGTH ) + " " +
                      FIELD_INDEXKEY + "\n", false ) ;

      rc = cannotUpgradeIdxInfoCl.find().sort( { "ClFullName": 1 } ) ;
      while ( rc.next() )
      {
         var obj = rc.current().toObj() ;

         if ( IDX_TYPE_MISSING == obj.UpgradeIndexType )
         {
            writeCheckInfo( pad( id, idLen ) + " " +
                            pad( obj.UpgradeIndexType, maxIndexTypeLength ) + " " +
                            pad( obj.ClFullName, maxClFullNameLength ) + " " +
                            pad( obj.IndexName, maxIndexNameLength ) + " " +
                            pad( getIdxAttrDesc( obj.IndexDef ), MAX_INDEX_ATTR_DESC_LENGTH ) + " " +
                            JSON.stringify( obj.IndexDef.key ) + "\n", false ) ;

            generateMissIdxDetailInfo( id, obj, missIdxDetail ) ;
         }
         else if ( IDX_TYPE_CONFLICT == obj.UpgradeIndexType )
         {
            if ( undefined != obj.IndexNames )
            {
               for ( i in obj.IndexNames )
               {
                  var indexName = obj.IndexNames[i] ;
                  writeCheckInfo( pad( id, idLen ) + " " +
                                  pad( obj.UpgradeIndexType, maxIndexTypeLength ) + " " +
                                  pad( obj.ClFullName, maxClFullNameLength ) + " " +
                                  pad( indexName, maxIndexNameLength ) + " " +
                                  pad( getIdxAttrDesc( obj.IndexDef ), MAX_INDEX_ATTR_DESC_LENGTH ) + " " +
                                  JSON.stringify( obj.IndexDef.key ) + "\n", false ) ;
               }
            }
            else if ( undefined != obj.IndexDefs )
            {
               for ( i in obj.IndexDefs )
               {
                  var indexDef = obj.IndexDefs[i] ;
                  writeCheckInfo( pad( id, idLen ) + " " +
                                  pad( obj.UpgradeIndexType, maxIndexTypeLength ) + " " +
                                  pad( obj.ClFullName, maxClFullNameLength ) + " " +
                                  pad( obj.IndexName, maxIndexNameLength ) + " " +
                                  pad( getIdxAttrDesc( indexDef ), MAX_INDEX_ATTR_DESC_LENGTH ) + " " +
                                  JSON.stringify( indexDef.key ) + "\n", false ) ;
               }
            }

            generateConflictIdxDetailInfo( id, obj, conflictIdxDetail,
                                           maxIndexNameLength, maxConflictIndexKeyLength ) ;
         }
         else if ( IDX_TYPE_INVALID_SHARD == obj.UpgradeIndexType )
         {
            writeCheckInfo( pad( id, idLen ) + " " +
                            pad( obj.UpgradeIndexType, maxIndexTypeLength ) + " " +
                            pad( obj.ClFullName, maxClFullNameLength ) + " " +
                            pad( obj.IndexName, maxIndexNameLength ) + " " +
                            pad( getIdxAttrDesc( obj.IndexDef ), MAX_INDEX_ATTR_DESC_LENGTH ) + " " +
                            JSON.stringify( obj.IndexDef.key ) + "\n", false ) ;

            generateInvalShardIdIdxDetailInfo( id, obj, invalidShardIdxDetail, maxClFullNameLength ) ;
            // ignore $shard
         }
         else if ( IDX_TYPE_INVALID_ID == obj.UpgradeIndexType )
         {
            writeCheckInfo( pad( id, idLen ) + " " +
                            pad( obj.UpgradeIndexType, maxIndexTypeLength ) + " " +
                            pad( obj.ClFullName, maxClFullNameLength ) + " " +
                            pad( obj.IndexName, maxIndexNameLength ) + " " +
                            pad( getIdxAttrDesc( obj.IndexDef ), MAX_INDEX_ATTR_DESC_LENGTH ) + " " +
                            JSON.stringify( obj.IndexDef.key ) + "\n", false ) ;

            generateInvalidIdIdxDetailInfo( id, obj, invalidIdIdxDetail, maxClFullNameLength ) ;
         }
         else if ( IDX_TYPE_LOCAL_IDX == obj.UpgradeIndexType )
         {
            writeCheckInfo( pad( id, idLen ) + " " +
                            pad( obj.UpgradeIndexType, maxIndexTypeLength ) + " " +
                            pad( obj.ClFullName, maxClFullNameLength ) + " " +
                            pad( obj.IndexDef.name, maxIndexNameLength ) + " " +
                            pad( getIdxAttrDesc( obj.IndexDef ), MAX_INDEX_ATTR_DESC_LENGTH ) + " " +
                            JSON.stringify( obj.IndexDef.key ) + "\n", false ) ;

            generateLocalClIdxDetailInfo( id, obj, localclIdxDetail, maxClFullNameLength ) ;
         }
         else if ( IDX_TYPE_INVALID_UNI == obj.UpgradeIndexType )
         {
            writeCheckInfo( pad( id, idLen ) + " " +
                            pad( obj.UpgradeIndexType, maxIndexTypeLength ) + " " +
                            pad( obj.ClFullName, maxClFullNameLength ) + " " +
                            pad( obj.IndexName, maxIndexNameLength ) + " " +
                            pad( getIdxAttrDesc( obj.IndexDef ), MAX_INDEX_ATTR_DESC_LENGTH ) + " " +
                            JSON.stringify( obj.IndexDef.key ) + "\n", false ) ;

            generateInvalidUniqueIdxDetailInfo( id, obj, invalidUniqueIdxDetail, maxClFullNameLength,
                                                maxIndexNameLength, maxInvalidUniqueIndexKeyLength ) ;
         }
         else
         {
            // do nothing
         }

         id++ ;
      }

      writeCheckInfo( "\n", false ) ;

      for ( i in missIdxDetail )
      {
         writeCheckInfo( "  " + "---------- Index ( ID: " + missIdxDetail[i].ID + " ) " +
                         IDX_TYPE_MISSING + " -----------\n" + "  " +
                         pad( FIELD_GROUPNAME, MAX_GROUP_NAME_LENGTH ) + " " +
                         pad( FIELD_NODENAME, MAX_NODE_NAME_LENGTH )  + " " +
                         pad( IDX_TYPE_MISSING, IDX_TYPE_MISSING.length ) + "\n" +
                         missIdxDetail[i].Detail + "\n", false ) ;
      }

      for ( i in conflictIdxDetail )
      {
         writeCheckInfo( "  " + "---------- Index ( ID: " + conflictIdxDetail[i].ID + " ) " +
                         IDX_TYPE_CONFLICT + " -----------\n" + "  " +
                         pad( FIELD_INDEXNAME, maxIndexNameLength )  + " " +
                         pad( FIELD_INDEXKEY, maxConflictIndexKeyLength )   + " " +
                         pad( FIELD_INDEXATTR, MAX_INDEX_ATTR_DESC_LENGTH )  + " " +
                         pad( FIELD_GROUPNAME, MAX_GROUP_NAME_LENGTH ) + "  " +
                         FIELD_NODENAME + "\n" + conflictIdxDetail[i].Detail + "\n", false ) ;
      }

      for ( i in invalidShardIdxDetail )
      {
         writeCheckInfo( "  " + "---------- Index ( ID: " + invalidShardIdxDetail[i].ID + " ) " +
                         IDX_TYPE_INVALID_SHARD + " -----------\n" + "  " +
                         pad( FIELD_COLLECTION, maxClFullNameLength ) + " " +
                         FIELD_NODENAME + "\n" + invalidShardIdxDetail[i].Detail + "\n", false ) ;
      }

      for ( i in invalidIdIdxDetail )
      {
         writeCheckInfo( "  " + "---------- Index ( ID: " + invalidIdIdxDetail[i].ID + " ) " +
                         IDX_TYPE_INVALID_ID + " -----------\n" + "  " +
                         pad( FIELD_COLLECTION, maxClFullNameLength ) + " " +
                         FIELD_NODENAME + "\n" + invalidIdIdxDetail[i].Detail + "\n", false ) ;
      }

      for ( i in localclIdxDetail )
      {
         writeCheckInfo( "  " + "---------- Index ( ID: " + localclIdxDetail[i].ID + " ) " +
                         localclIdxDetail[i].InvalidType + " CL -----------\n" + "  " +
                         pad( FIELD_COLLECTION, maxClFullNameLength ) + " " +
                         FIELD_NODENAME + "\n" + localclIdxDetail[i].Detail + "\n", false ) ;
      }

      for ( i in invalidUniqueIdxDetail )
      {
         writeCheckInfo( "  " + "---------- Index ( ID: " + invalidUniqueIdxDetail[i].ID + " ) " +
                         IDX_TYPE_INVALID_UNI + " -----------\n" + "  " +
                         pad( FIELD_COLLECTION, maxClFullNameLength ) + " " +
                         pad( FIELD_INDEXNAME, maxIndexNameLength ) + " " +
                         pad( FIELD_INDEXKEY, maxInvalidUniqueIndexKeyLength ) + " " +
                         FIELD_MAIN_AND_SUB_CL_SHARDKEY + "\n" +
                         invalidUniqueIdxDetail[i].Detail + "\n", false ) ;
      }

      writeCheckInfo( "", true ) ;

      println( "(" + step + "/" + STEP_NUM + ")End to write cannot upgrade index infos, spent time: " +
               ( (Date.now()) - beginTime )/1000 + "s" ) ;
      step++ ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to write cannot upgrade index infos, error Stack:\n" +
                  e.stack ) ;
      }
      throw e ;
   }
}

function writeInvalidClReport( invalidClInfoCl, invalidClCount )
{
   try
   {
      var beginTime = Date.now() ;

      println( "(" + step + "/" + STEP_NUM + ")Begin to write invalid collection inofs" ) ;

      var id = 1 ;
      var idLen = ( invalidClCount + "" ).length ;
      idLen = Math.max( idLen, FIELD_ID.length + 1 ) ;

      var maxClFullNameLength = getMaxCannotClFullNameLength( invalidClInfoCl ) ;

      writeCheckInfo( "===================== Check Result ( Invalid CL ) ======================\n" +
                      pad( FIELD_ID, idLen ) + " " +
                      pad( FIELD_COLLECTION, maxClFullNameLength )   + " " +
                      pad( FIELD_INVALID_TYPE, MAX_INVALID_CL_TYPE_LENGTH ) + " " +
                      pad( FIELD_UNIQUEID, MAX_UNIQUEID_LENGTH ) + " " +
                      pad( FIELD_RECORD_COUNT, MAX_RECORD_OR_LOB_COUNT_LENGTH ) + " " +
                      pad( FIELD_LOB_COUNT, MAX_RECORD_OR_LOB_COUNT_LENGTH ) + " " +
                      pad( FIELD_GROUPNAME, MAX_GROUP_NAME_LENGTH ) + " " +
                      FIELD_NODENAME + "\n", false ) ;

      var rc = invalidClInfoCl.find() ;
      while( rc.next() )
      {
         var obj = rc.current().toObj() ;
         writeCheckInfo( pad( id, idLen ) + " " +
                         pad( obj.ClFullName, maxClFullNameLength )   + " " +
                         pad( obj.InvalidType, MAX_INVALID_CL_TYPE_LENGTH ) + " " +
                         pad( obj.UniqueID, MAX_UNIQUEID_LENGTH ) + " " +
                         pad( obj.RecordCount, MAX_RECORD_OR_LOB_COUNT_LENGTH ) + " " +
                         pad( obj.LobCount, MAX_RECORD_OR_LOB_COUNT_LENGTH ) + " " +
                         pad( obj.GroupName, MAX_GROUP_NAME_LENGTH ) + " " +
                         obj.NodeName + "\n", false ) ;
         id++ ;
      }
      writeCheckInfo( "\n", true ) ;

      println( "(" + step + "/" + STEP_NUM + ")End to write invalid collection inofs, spent time: " +
               ( (Date.now()) - beginTime )/1000 + "s" ) ;
      step++ ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to write invalid collection inofs, error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}

function writeReport()
{
   var beginTime = Date.now() ;

   println( "(" + step + "/" + STEP_NUM + ")Begin to get indexes count" ) ;

   var tmpCS = db.getCS( TMP_CS_UPGRADE_INDEX ) ;
   var dataIdxCheckInfoCl = tmpCS.getCL( TMP_CL_DATA_INDEX_CHECK_INFO ) ;
   var mainIdxCheckInfoCl = tmpCS.getCL( TMP_CL_MAIN_CL_INDEX_CHECK_INFO ) ;
   var noNeedUpgradeIdxInfoCl = tmpCS.getCL( TMP_CL_NO_NEED_UPGRADE_INDEX_INFO ) ;
   var cannotUpgradeIdxInfoCl = tmpCS.getCL( TMP_CL_CANNOT_UPGRADE_INDEX_INFO ) ;
   var invalidClInfoCl = tmpCS.getCL( TMP_CL_INVALID_CL_INFO ) ;

   var noNeedIdxCount = ( dataIdxCheckInfoCl.count( { "UpgradeIndexType": IDX_TYPE_CONSISTENT } ) ) +
                        ( mainIdxCheckInfoCl.count( { "UpgradeIndexType": IDX_TYPE_CONSISTENT } ) ) +
                        ( noNeedUpgradeIdxInfoCl.count() ) ;
   var canUpgradeIdxCount = ( dataIdxCheckInfoCl.count( { "UpgradeIndexType": { "$isnull": 1 } } ) ) +
                            ( mainIdxCheckInfoCl.count( { "UpgradeIndexType": { "$isnull": 1 } } ) )  ;
   var cannotUpgradeIdxCount = cannotUpgradeIdxInfoCl.count() ;
   var invalidClCount = invalidClInfoCl.count() ;

   println( "(" + step + "/" + STEP_NUM + ")End to get indexes count, spent time: " +
            ( (Date.now()) - beginTime )/1000 + "s" ) ;
   step++ ;

   writeNoNeedUpgradeIdxReport( noNeedIdxCount, dataIdxCheckInfoCl, mainIdxCheckInfoCl,
                                noNeedUpgradeIdxInfoCl ) ;

   writeCanUpgradeIdxReport( canUpgradeIdxCount, dataIdxCheckInfoCl, mainIdxCheckInfoCl ) ;

   writeCannotUpgradeIdxReport( cannotUpgradeIdxCount, cannotUpgradeIdxInfoCl ) ;

   writeInvalidClReport( invalidClInfoCl, invalidClCount ) ;

   writeCheckInfo( SUGGEST_INFO +
                   "++++++++++++++++++++++++++\n" +
                   "No need to upgrade : " + noNeedIdxCount + "\n" +
                   "Cannot be upgraded : " + cannotUpgradeIdxCount + "\n" +
                   "   Can be upgraded : " + canUpgradeIdxCount + "\n" +
                   "  Invalid cl count : " + invalidClCount + "\n" +
                   "++++++++++++++++++++++++++\n", true ) ;
}

function generateJsScripts()
{
   generateCanUpgradeIdxJsScript() ;

   generateCannotUpgradeIdxJsScripts() ;
}

function defineNewCreateIndexFunc()
{
   var code = "" ;
   code += "function createIndex( clFullName, collection, indexName, indexKey, indexDef, supportOnlyUpgradeMeta ) {\n" ;
   code += "   try {\n" ;
   code += "      if ( supportOnlyUpgradeMeta ) {\n" ;
   code += "         collection.createIndex( indexName, indexKey, indexDef, { 'OnlyUpgradeMeta': true } ) ;\n" ;
   code += "      }\n"
   code += "      else {\n" ;
   code += "         collection.createIndex( indexName, indexKey, indexDef ) ;\n" ;
   code += "      }\n" ;
   code += "      println( 'Create index successfully[cl: ' + clFullName + ', indexName: ' + indexName + ']' ) ;\n" ;
   code += "   }\n" ;
   code += "   catch( e ) {\n" ;
   code += "      if ( e != -177 ) {\n" ;
   code += "         println( 'Failed to create index[cl: ' + clFullName + ', indexName: ' + indexName + '], rc: ' + e ) ;\n" ;
   code += "         throw e ;\n" ;
   code += "      }\n" ;
   code += "      println( 'Can not create index[cl: ' + clFullName + ', indexName: ' + indexName + '], rc: ' + e ) ;\n" ;
   code += "   }\n" ;
   code += "}\n" ;
   return code ;
}

function defineNewEnableShardingFunc()
{
   var code = "" ;
   code += "function enableSharding( clFullName, indexName, collection, indexKey ) {\n" ;
   code += "   try {\n" ;
   code += "      collection.enableSharding( indexKey ) ;\n" ;
   code += "      println( 'Create index successfully[cl: ' + clFullName + ', indexName: ' + indexName + ']' ) ;\n" ;
   code += "   }\n" ;
   code += "   catch( e ) {\n" ;
   code += "      if ( e != -177 ) {\n" ;
   code += "         println( 'Failed to create index[cl: ' + clFullName + ', indexName: ' + indexName + '], rc: ' + e ) ;\n" ;
   code += "         throw e ;\n" ;
   code += "      }\n" ;
   code += "      println( 'Can not create index[cl: ' + clFullName + ', indexName: ' + indexName + '], rc: ' + e ) ;\n" ;
   code += "   }\n" ;
   code += "}\n" ;
   return code ;
}

function generateCanUpgradeIdxJsScript()
{
   try
   {
      var rc = null ;
      var id = 1 ;
      var beginTime = Date.now() ;

      var tmpCS = db.getCS( TMP_CS_UPGRADE_INDEX ) ;
      var dataIdxCheckInfoCl = tmpCS.getCL( TMP_CL_DATA_INDEX_CHECK_INFO ) ;
      var mainIdxCheckInfoCl = tmpCS.getCL( TMP_CL_MAIN_CL_INDEX_CHECK_INFO ) ;
      var canUpgradeIdxCount = ( dataIdxCheckInfoCl.count( { "UpgradeIndexType": { "$isnull": 1 } } ) ) +
                               ( mainIdxCheckInfoCl.count( { "UpgradeIndexType": { "$isnull": 1 } } ) )  ;

      var printProgressCount = _PRINT_PROGRESS_COUNT ;
      var printClTmpNum = canUpgradeIdxCount/printProgressCount ;
      printClTmpNum = printClTmpNum > 1 ? printClTmpNum : 1 ;
      var aProgressCollectClNum = printClTmpNum ;

      var writeProgressCount = canUpgradeIdxCount ;
      var writeJsCodeTmpNum = WRITE_JS_FILE_BATCH_NUM ;
      var aProgressWriteJsCodeNum = writeJsCodeTmpNum ;

      var tmpCS = db.getCS( TMP_CS_UPGRADE_INDEX ) ;
      var dataIdxCheckInfoCl = tmpCS.getCL( TMP_CL_DATA_INDEX_CHECK_INFO ) ;
      var mainIdxCheckInfoCl = tmpCS.getCL( TMP_CL_MAIN_CL_INDEX_CHECK_INFO ) ;

      if ( 0 == dataIdxCheckInfoCl.count( { "UpgradeIndexType": { "$isnull": 1 } } ) &&
           0 == mainIdxCheckInfoCl.count( { "UpgradeIndexType": { "$isnull": 1 } } ) )
      {
         println( "(" + step + "/" + STEP_NUM + ")Don't need to generate can upgrade indexes js script" ) ;
         step++ ;
         return ;
      }

      println( "(" + step + "/" + STEP_NUM + ")Begin to generate can upgrade indexes js script" ) ;
      // 每完成四十分之一打印一个 #
      println( "Writing\n0% ______________ 50% ______________ 100%" ) ;

      writeUpgradeJsCode( UPGRADE_INDEX_JS_FILE,
                          "var beginTime = Date.now() ;\n" +
                          "var spentTime = 0 ;\n" +
                          "var everyIndexSpentTime = 0 ;\n" +
                          "var everyIndexSpentTimeTmp = 0 ;\n" +
                          "var leftTime = 0 ;\n" +
                          "var leftTimeTmp = 0 ;", false ) ;
      writeUpgradeJsCode( UPGRADE_INDEX_JS_FILE,
                          generateSdbConnStr( HOSTNAME, SVCNAME, USERNAME, PASSWD, CIPHER_FILE, TOKEN ),
                          false ) ;
      writeUpgradeJsCode( UPGRADE_INDEX_JS_FILE,
                          defineNewCreateIndexFunc(),
                          false ) ;
      writeUpgradeJsCode( UPGRADE_INDEX_JS_FILE,
                          defineNewEnableShardingFunc(),
                          false ) ;

      rc = dataIdxCheckInfoCl.find( { "UpgradeIndexType": { "$isnull": 1 } } ).sort( { "ClFullName": 1 } ) ;
      while ( rc.next() )
      {
         var obj = rc.current().toObj() ;

         writeUpgradeJsCode( UPGRADE_INDEX_JS_FILE, generateUpgradeIdxJsCode( obj ), false ) ;
         if ( id >= writeJsCodeTmpNum && 0 != writeProgressCount )
         {
            var progressCode = "spentTime = ( (Date.now()) - beginTime )/1000 ;\n" ;
            progressCode += "everyIndexSpentTimeTmp = spentTime/" + id + " ;\n" ;
            progressCode += "everyIndexSpentTime = parseFloat( everyIndexSpentTimeTmp.toFixed( 3 ) ) ;\n" ;
            progressCode += "leftTimeTmp = everyIndexSpentTime * " + ( canUpgradeIdxCount - id ) + " ;\n" ;
            progressCode += "leftTime = parseFloat( leftTimeTmp.toFixed( 3 ) ) ;\n" ;
            // [2/300] Cost: xxxs, Speed: xxxs/index, Left: xxxs
            progressCode +=
            ( "println('[" + id + "/" + canUpgradeIdxCount + "] Cost: ' + spentTime + 's, " +
               "Speed: ' + everyIndexSpentTime + 's/index, " +
               "Left time: ' + leftTime + 's' ) ;\n" ) ;
            writeUpgradeJsCode( UPGRADE_INDEX_JS_FILE, progressCode, false ) ;
            writeJsCodeTmpNum += aProgressWriteJsCodeNum ;
            writeProgressCount -= id ;
         }

         id++ ;
         if ( id >= printClTmpNum && 0 != printProgressCount )
         {
            print( "#" ) ;
            printClTmpNum += aProgressCollectClNum ;
            printProgressCount-- ;
         }
      }

      rc = mainIdxCheckInfoCl.find( { "UpgradeIndexType": { "$isnull": 1 } } ).sort( { "ClFullName": 1 } ) ;
      while ( rc.next() )
      {
         var obj = rc.current().toObj() ;

         writeUpgradeJsCode( UPGRADE_INDEX_JS_FILE, generateUpgradeIdxJsCode( obj ), false ) ;
         if ( id >= writeJsCodeTmpNum && 0 != writeProgressCount )
         {
            var progressCode = "spentTime = ( (Date.now()) - beginTime )/1000 ;\n" ;
            progressCode += "everyIndexSpentTimeTmp = spentTime/" + id + " ;\n" ;
            progressCode += "everyIndexSpentTime = parseFloat( everyIndexSpentTimeTmp.toFixed( 3 ) ) ;\n" ;
            progressCode += "leftTimeTmp = everyIndexSpentTime * " + ( canUpgradeIdxCount - id ) + " ;\n" ;
            progressCode += "leftTime = parseFloat( leftTimeTmp.toFixed( 3 ) ) ;\n" ;
            progressCode +=
            ( "println('[" + id + "/" + canUpgradeIdxCount + "] Cost: ' + spentTime + 's, " +
              "Speed: ' + everyIndexSpentTime + 's/index, " +
              "Left time: ' + leftTime + 's' ) ;\n" ) ;
            writeUpgradeJsCode( UPGRADE_INDEX_JS_FILE, progressCode, false ) ;
            writeJsCodeTmpNum += aProgressWriteJsCodeNum ;
            writeProgressCount -= id ;
         }

         id++ ;
         if ( id >= printClTmpNum && 0 != printProgressCount )
         {
            print( "#" ) ;
            printClTmpNum += aProgressCollectClNum ;
            printProgressCount-- ;
         }
      }

      id-- ;
      if ( 0 != writeProgressCount )
      {
         var progressCode = "spentTime = ( (Date.now()) - beginTime )/1000 ;\n" ;
         progressCode += "everyIndexSpentTimeTmp = spentTime/" + id + " ;\n" ;
         progressCode += "everyIndexSpentTime = parseFloat( everyIndexSpentTimeTmp.toFixed( 3 ) ) ;\n" ;
         progressCode += "leftTimeTmp = everyIndexSpentTime * " + ( canUpgradeIdxCount - id ) + " ;\n" ;
         progressCode += "leftTime = parseFloat( leftTimeTmp.toFixed( 3 ) ) ;\n" ;
         progressCode +=
         ( "println('[" + id + "/" + canUpgradeIdxCount + "] Cost: ' + spentTime + 's, " +
           "Speed: ' + everyIndexSpentTime + 's/index, " +
           "Left time: ' + leftTime + 's' ) ;\n" ) ;
         writeUpgradeJsCode( UPGRADE_INDEX_JS_FILE, progressCode, false ) ;
      }
      writeUpgradeJsCode( UPGRADE_INDEX_JS_FILE, "db.close() ;\n", true ) ;

      for ( var i = 0 ; i < printProgressCount ; i++ )
      {
         print( "#" ) ;
      }
      print( "\n" ) ;

      println( "(" + step + "/" + STEP_NUM + ")End to generate can upgrade indexes js script, spent time: " +
               ( (Date.now()) - beginTime )/1000 + "s" ) ;
      step++ ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to generate can upgrade indexes js script, error Stack:\n" +
                  e.stack ) ;
      }
      throw e ;
   }
}

function generateCannotUpgradeIdxJsScripts()
{
   try
   {
      var beginTime = Date.now() ;
      var tmpCS = db.getCS( TMP_CS_UPGRADE_INDEX ) ;
      var cannotUpgradeIdxInfoCl = tmpCS.getCL( TMP_CL_CANNOT_UPGRADE_INDEX_INFO ) ;

      if ( 0 == cannotUpgradeIdxInfoCl.count() )
      {
         println( "(" + step + "/" + STEP_NUM + ")Don't need to generate cannot upgrade indexes js script" ) ;
         step++ ;
         return ;
      }

      println( "(" + step + "/" + STEP_NUM + ")Begin to generate cannot upgrade indexes js script" ) ;

      rc = cannotUpgradeIdxInfoCl.find().sort( { "ClFullName": 1 } ) ;
      while ( rc.next() )
      {
         var obj = rc.current().toObj() ;

         if ( IDX_TYPE_MISSING == obj.UpgradeIndexType )
         {
            writeFile( MISS_INDEX_JS_FILE, generateMissIdxJsCode( obj ), false ) ;
         }
         else if ( IDX_TYPE_CONFLICT == obj.UpgradeIndexType )
         {
            writeFile( CONFLICT_INDEX_JS_FILE, generateConflictIdxJsCode( obj ), false ) ;
         }
         else if ( IDX_TYPE_INVALID_SHARD == obj.UpgradeIndexType )
         {
            // ignore $shard
         }
         else if ( IDX_TYPE_INVALID_ID == obj.UpgradeIndexType )
         {
            writeFile( INVALID_ID_INDEX_JS_FILE, generateInvalidIdIdxJsCode( obj ), false ) ;
         }
         else if ( IDX_TYPE_LOCAL_IDX == obj.UpgradeIndexType )
         {
            // do nothing
         }
         else
         {
            // do nothing
         }
      }

      writeAndgenerateLocalClJsCode() ;

      println( "(" + step + "/" + STEP_NUM + ")End to generate cannot upgrade indexes js script, spent time: " +
               ( (Date.now()) - beginTime )/1000 + "s" ) ;
      step++ ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( "Failed to generate cannot upgrade indexes js script, error Stack:\n" +
                  e.stack ) ;
      }
      throw e ;
   }
}

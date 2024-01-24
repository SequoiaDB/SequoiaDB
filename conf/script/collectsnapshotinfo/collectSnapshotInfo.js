/*******************************************************************************

   Copyright (C) 2016-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*******************************************************************************/

/*
 * parse {a:1, b:[1, 2, 3]} to [{a:1, b:1}, {a:1, b:2}, {a:1, b:3}]
 */
function parseArray2Object( record )
{
   var retArr = new Array() ;

   for ( var key in record )
   {
      var newArr = new Array() ;
      var value  = record[key] ;
      // some fields do not need to be handled, let's ignore them
      for ( var i = 0 ; i < IGNORE_FIELDS.length; i++ )
      {
         if ( key == IGNORE_FIELDS[i] && isObject(value) )
         {
            value = JSON.stringify( value ) ;
            break ;
         }
      }
      // begine to parse
      if ( !isObject(value) )
      {
         // element is general value
         newArr.push( new Object() ) ;
         newArr[0][key] = value ;
      }
      else if ( isArray(value) ) 
      {
         // element is array
         for ( var i = 0; i < value.length; i++ )
         {
            var subObj = new Object() ;
            var subArr = null ;
            var elem   = value[i] ;
            subObj[key] = elem ;
            subArr = parseArray2Object( subObj ) ;
            // merge subArr into newArr
            newArr = newArr.concat( subArr ) ;
         }
      }
      else
      {
         // element is object
         var arr = parseArray2Object( value ) ; 
         for ( var i = 0; i < arr.length; i++ )
         {
            var tmpObj = new Object() ;
            tmpObj[key] = arr[i] ;
            newArr.push( tmpObj ) ;
         }
      }
      // merge the original elements with the new elements
      if ( newArr.length != 0 && retArr.length != 0 )
      {
         var arr = new Array() ;
         for ( var i = 0; i < retArr.length; i++ )
         {
            for ( var j = 0; j < newArr.length; j++ )
            {
               var tmpObj = new Object() ;
               for( var k in retArr[i] )
               {
                  tmpObj[k] = retArr[i][k] ;
               }
               for( var k in newArr[j] )
               {
                  tmpObj[k] = newArr[j][k] ;
               }
               arr.push( tmpObj ) ;
            }
         }
         retArr = arr.slice( 0, arr.length ) ;
      }
      else if ( newArr.length != 0 )
      {
         for( var i = 0; i < newArr.length; i++ )
         {
            retArr.push( newArr[i] ) ;
         }
      }
   }

   return retArr ;
}

function flatRecord( record ) 
{
   var retObj = new Object() ;
   for( var key in record )
   {
      var tmpObj = null ;
      var value = record[key] ;
      // some fields do not need to be handled, let's ignore them
      for ( var i = 0 ; i < IGNORE_FIELDS.length; i++ )
      {
         if ( key == IGNORE_FIELDS[i] && isObject(value) )
         {
            value = JSON.stringify( value ) ;
            break ;
         }
      }
      // begine to flat
      if( isObject( value ) )
      {
         tmpObj = flatRecord( value ) ;
      }
      if ( tmpObj != null )
      {
         for ( var subKey in tmpObj )
         {
            retObj[key + subKey] = tmpObj[subKey] ;
         }
      }
      else
      {
         if ( key == "UserCPU" || key == "SysCPU" )
         {
            retObj[key] = parseFloat( value ) ;
         }
         else
         {
            retObj[key] = value ;
         }
      }
   }
   return retObj ;
} ;

function transformRecord( record, timestamp )
{
   var exp    = null ;
   var tmpArr = null ;
   var retArr = [] ;
   
   // remove _id field
   try
   {
      delete record._id ;
   }
   catch( e )
   {
      exp = new SdbError( e, 
         sprintf( "failed to remove _id field in record[?]", 
            JSON.stringify(record) ) ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
   // parse array elements to object elements
   try
   {
      tmpArr = parseArray2Object( record ) ;   
   }
   catch( e )
   {
      exp = new SdbError( e, sprintf( "failed to parse array elements " + 
         "to object elements in record[?]", JSON.stringify(record) ) ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
   // check the result
   if ( !isArray(tmpArr) )
   {
      var exp = new SdbError( SDB_SYS, 
         sprintf( "the parsing result[?] is not an array", tmpArr ) ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
   // flat the record and process it
   for ( var i = 0; i < tmpArr.length; i++ )
   {
      var tmpRecord = flatRecord( tmpArr[i] ) ;
      // add timestamp field
      tmpRecord[TimeStamp] = timestamp ;
      retArr.push( tmpRecord ) ;
   }
   return retArr ;
}

function insertLogInfo( cl, info )
{
   if ( !isObject(info) )
   {
      var exp = new SdbError( SDB_SYS, 
         sprintf( "log info[?] is not an object", info ) ) ;
      logger.log( PDERROR, exp ) ;
      return -1 ;
   }
   // insert
   try
   {
      info.ErrNodes = info.ErrNodes.toString() ;
      logger.log( PDDEBUG, 
         sprintf( "log info is: [?]", JSON.stringify(info) ) ) ;
      cl.insert( info ) ;
   }
   catch( e )
   {
      var exp = new SdbError( e, 
         sprintf( "failed to insert log info into collection[?]", cl ) ) ;
      logger.log( PDERROR, exp ) ;
      return -1 ;
   }
   return 0 ;
}

function insertListInfo( db, listType, timestamp ) 
{
   var logInfo = new LogInfo() ;
   // get cl
   var clName  = getTable( OP_TYPE_GET_LIST, listType ) ;
   var cl      = db.getCS( CS_CMBC_STAT ).getCL( clName ) ;
   var cl2     = db.getCS( CS_CMBC_STAT ).getCL( CL_LOG_INFO ) ;
   // prepare log info
   logInfo.OperationType = OP_TYPE_GET_LIST ;
   logInfo.Operation     = listType ;
   logInfo.TimeStamp     = timestamp ;
   
   try
   {
      var cur        = null ;
      var record     = null ;
      var newRecords = null ;
      // get list info
      cur    = db.list( listType ) ;
      record = cur.next() ;
      // extract info
      while( typeof(record) != "undefined" )
      {
         newRecords = transformRecord( record.toObj(), timestamp ) ; 
         // insert
         cl.insert( newRecords ) ;
         record = cur.next() ;
      }
   } 
   catch( e ) 
   {
      var exp = new SdbError( e, 
         sprintf( "failed to insert list[?] info into " + 
            "collection[?] at timestamp[?]", listType, cl, timestamp ) ) ;
      logger.log( PDERROR, exp ) ;
      logInfo.ErrNo = exp.getErrCode() ;
      logInfo.ErrNodes.push( sprintf( "?(?)", db.toString(), logInfo.ErrNo ) ) ;
   }
   // insert log info into collection
   return insertLogInfo( cl2, logInfo ) ;
} ;

function insertSnapshotInfo( db, snapType, nodeArr, timestamp ) 
{
   var exp     = null ;
   var logInfo = new LogInfo() ;
   // get cl
   var clName  = getTable( OP_TYPE_GET_SNAPSHOT, snapType ) ;
   var cl      = db.getCS( CS_CMBC_STAT ).getCL( clName ) ;
   var cl2     = db.getCS( CS_CMBC_STAT ).getCL( CL_LOG_INFO ) ;
   // prepare log info
   logInfo.OperationType = OP_TYPE_GET_SNAPSHOT ;
   logInfo.Operation     = snapType ;
   logInfo.TimeStamp     = timestamp ;
   for ( var i = 0; i < nodeArr.length; i++ )
   {
      var hostName = null ;
      var svcName  = null ;
      var ddb      = null ;
      try 
      {
         // get node host name and service name
         hostName  = nodeArr[i].hostName ;
         svcName   = nodeArr[i].svcName ;
      }
      catch( e )
      {
         exp = new SdbError( e, 
            sprintf( "failed to get host name[?] and service name[?] " +
               "from node info[?]", hostName, svcName, nodeArr[i] ) ) ;
         logger.log( PDERROR, exp ) ;
         return -1 ;
      }
      try
      {
         var cur        = null ;
         var record     = null ;
         var newRecords = null ;
         // connect to data node
         ddb       = new Sdb( hostName, svcName, USER_NAME, PASSWORD ) ;
         // get snapshot
         cur       = ddb.snapshot( snapType ) ;
         record    = cur.next() ;
         // extract info
         while( typeof(record) != "undefined" )
         {
            newRecords = transformRecord( record.toObj(), timestamp ) ; 
            // insert
            cl.insert( newRecords ) ;
            record = cur.next() ;
         }
      } 
      catch( e ) 
      {
         exp = new SdbError( e, 
            sprintf( "failed to insert snapshot[?] info of node[?] into " + 
               "collection[?] at timestamp[?]", snapType, 
               ( ( ddb == null ) ? null : ddb.toString() ), cl, timestamp ) ) ;
         logger.log( PDERROR, exp ) ;
         logInfo.ErrNo = exp.getErrCode() ;
         logInfo.ErrNodes.push( 
            sprintf( "?:?(?)", hostName, svcName, logInfo.ErrNo ) ) ;
         continue ;
      }
      finally
      {
         try{ ddb.close() ; } catch( e ) {}
      }
   }
   // insert log info into collection
   return insertLogInfo( cl2, logInfo ) ;
} ;

function processList( db, listType, timestamp )
{
   if ( !isObject(db) )
   {
      logger.log( PDERROR, 
         sprintf( "the input db[?] is not an object", db ) ) ;
      return -1 ;      
   }
   if ( !isNumber(listType) ) 
   {
      logger.log( PDERROR,
         sprintf( "the input list type[?] is not a number", listType ) ) ;
      return -1 ;
   }
   if ( listType < 0 || listType >= LIST_TYPE_MAX ) 
   {
      logger.log( PDERROR,
         sprintf( "the input list type[?] is invalid", listType ) ) ;
      return -1 ;
   }
   if ( !isNumber(timestamp) )
   {
      logger.log( PDERROR, 
         sprintf( "the input timestamp[?] is not a number", timestamp ) ) ;
      return -1 ;
   }
   try 
   {
      return insertListInfo( db, listType, timestamp ) ;
   } 
   catch( e ) 
   {
      var exp = SdbError( e, "failed to insert list info" ) ;
      logger.log( PDERROR, exp ) ;
      return -1 ;
   }
} ;

function processSnapshot( db, snapType, nodeArr, timestamp )
{
   if ( !isObject(db) )
   {
      logger.log( PDERROR, 
         sprintf( "the input db[?] is not an object", db ) ) ;
      return -1 ;      
   }
   if ( !isNumber(snapType) ) 
   {
      logger.log( PDERROR,
         sprintf( "the input snapshot type[?] is not a number", snapType ) ) ;
      return -1 ;
   }
   if ( snapType < 0 || snapType >= SNAPSHOT_TYPE_MAX ) 
   {
      logger.log( PDERROR,
         sprintf( "the input snapshot type[?] is invalid", snapType ) ) ;
      return -1 ;
   }
   if ( !isArray(nodeArr) ) 
   {
      logger.log( PDERROR, 
         sprintf( "the input node array[?] is not an array", nodeArr ) ) ;
      return -1 ;
   }
   if ( !isNumber(timestamp) )
   {
      logger.log( PDERROR, 
         sprintf( "the input timestamp[?] is not a number", timestamp ) ) ;
      return -1 ;
   }
   try 
   {
      return insertSnapshotInfo( db, snapType, nodeArr, timestamp ) ;
   } 
   catch( e ) 
   {
      var exp = SdbError( e, "failed to insert snapshot info" ) ;
      logger.log( PDERROR, exp ) ;
      return -1 ;
   }
} ;

function getTable( operationType, type )
{
   var snapshotTable = ["", "", "", "", "CL_CL_DETAIL_INFO", "", "CL_DATABASE_INFO", "CL_SYSTEM_INFO", "", "", "", "", "", "", "CL_TASK_INFO" ] ;
   // TODO: add the actual table name here
   var listTable = ["", "", "", "", "CL_CL_LIST_INFO", "CL_CS_LIST_INFO", "", "CL_GROUP_LIST_INFO", "", "", "", "", "", "", "" ] ;
   if ( OP_TYPE_GET_SNAPSHOT == operationType )
   {
      if ( type < 0 || type >= SNAPSHOT_TYPE_MAX )
      {
         return null ;
      }
      return snapshotTable[type] ;
   }
   else if ( OP_TYPE_GET_LIST == operationType )
   {
      if ( type < 0 || type >= LIST_TYPE_MAX )
      {
         return null ;
      }
      return listTable[type] ;
   }
}

function getdb()
{
   var i = 0 ;
   for ( i = 0; i < COORD_LIST.length ; ++i )
   {
      try
      {
         var db ;
         db = new Sdb( COORD_LIST[i].hostName, COORD_LIST[i].svcName, USER_NAME, PASSWORD ) ;
         return db ;
      }
      catch( e )
      {
         logger.log( PDINFO, "Connect failed: " + COORD_LIST[i].hostName + ":" + COORD_LIST[i].svcName ) ;
      }
   }
   var exp = SdbError( "All coord-node is connected failed!" ) ;
   logger.log(PDERROR, "Get db failed: " + exp ) ;
   throw exp ;
}

function getRGList( db )
{
   if ( typeof( RG_LIST ) != "object" )
   {
      try
      {
         var tmpInfo = db.listReplicaGroups().toArray() ;
         RG_LIST = new Array() ;
         for( var i = 0 ; i < tmpInfo.length ; ++i )
         {
            var tmpObj = eval( "(" + tmpInfo[i] + ")" ) ;
            RG_LIST[i] = new Object() ;
            RG_LIST[i].GroupName = tmpObj.GroupName ;
            RG_LIST[i].GroupID = tmpObj.GroupID ;
            RG_LIST[i].Status = tmpObj.Status ;
            RG_LIST[i].Version = tmpObj.Version ;
            RG_LIST[i].Role = tmpObj.Role ;
            
            var tmpGroupObj = tmpObj.Group ;
            RG_LIST[i].Group = new Array() ;
            for ( var j = 0 ; j < tmpGroupObj.length ; ++j )
            {
               var tmpNodeObj = tmpGroupObj[j] ;
               RG_LIST[i].Group[j] = new Object() ;
               RG_LIST[i].Group[j].HostName = tmpNodeObj.HostName ;
               RG_LIST[i].Group[j].dbpath = tmpNodeObj.dbpath ;
               RG_LIST[i].Group[j].NodeID = tmpNodeObj.NodeID ;
               var tmpSvc = tmpNodeObj.Service ;
               RG_LIST[i].Group[j].Service = new Array() ;
               for ( var k = 0 ; k < tmpSvc.length ; ++k  )
               {
                  RG_LIST[i].Group[j].Service[k] = new Object() ;
                  RG_LIST[i].Group[j].Service[k].Name = tmpSvc[k].Name ;
                  RG_LIST[i].Group[j].Service[k].Type = tmpSvc[k].Type ;
               }
            }
         }
      }
      catch( e )
      {
         RG_LIST = "" ;
         var exp = new SdbError( e, "Failed to list replica Groups!" ) ;
         logger.log( PDERROR, exp ) ;
         throw exp ;
      }
   }
   return RG_LIST ;
}

function getDataRGNameList( db )
{
   var rgList = getRGList( db ) ;
   var rgNameList = new Array() ;
   var j = 0 ;
   for ( var i = 0 ; i < rgList.length ; ++i )
   {
      if ( rgList[i].GroupName != "SYSCatalogGroup"
           && rgList[i].GroupName != "SYSCoord" )
      {
         rgNameList[j] = rgList[i].GroupName ;
         ++j ;
      }
   }
   if ( rgNameList.length <= 0 )
   {
      var exp = new SdbError( "Failed to get replica group list!") ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
   return rgNameList ;
}

function getCataDataAddrList( db )
{
   var rgList = getRGList( db ) ;
   var nodeAddrList = new Array() ;
   var i = 0 ;
   var j = 0 ;
   var k = 0 ;
   var m = 0 ;
   for ( i = 0 ; i < rgList.length ; ++i )
   {
      if ( rgList[i].GroupName != "SYSCoord" )
      {
         var nodes = rgList[i].Group ;
         for ( j = 0 ; j < nodes.length ; ++j )
         {
            for ( k = 0 ; k < nodes[j].Service.length ; ++k )
            {
               if ( nodes[j].Service[k].Type == 0 )
               {
                  nodeAddrList[m] = new Object() ;
                  nodeAddrList[m].hostName = nodes[j].HostName ;
                  nodeAddrList[m].svcName = nodes[j].Service[k].Name ;
                  ++m ;
                  break ;
               }
            }
         }
      }
   }
   if ( nodeAddrList.length <= 0 )
   {
      var exp = new SdbError( "Failed to get node address list!") ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
   return nodeAddrList ;
}

function createDomain( db, dmName )
{
   try
   {
      var rgNameList = getDataRGNameList( db ) ;
      db.createDomain( dmName, rgNameList, {AutoSplit:true} ) ;
      return 0 ;
   }
   catch ( e )
   {
      if ( e == -215 )
      {
         return 0 ;
      }
      var exp = new SdbError( e, "Failed to create domain!" ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
}

function createCS( db, csName, options )
{
   try
   {
      db.createCS( csName, options ) ;
      return 0 ;
   }
   catch ( e )
   {
      if ( e == -33 )
      {
         return 0 ;
      }
      var exp = new SdbError( e, "Failed to create cs!" ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
}

function createCL( db, csName, clName, options )
{
   try
   {
      eval( 'db.'+csName+'.createCL("'+clName+'",'+options+')') ;
      return 0 ;
   }
   catch ( e )
   {
      if ( e == -22 )
      {
         return 0 ;
      }
      var exp = new SdbError( e, "Failed to crate cl: "+csName+"."+clName +", options: "+options.ShardingKey ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
}

function createMainCL( db, csName, clName )
{
   try
   {
      var month = CUR_DATE.getMonth() + 1 ;
      var subCLName = clName + "_" + CUR_DATE.getFullYear() + "_" + month ;
      var beginDate = new Date( CUR_DATE.getFullYear(), CUR_DATE.getMonth(), 1, 0, 0, 0 ) ;
      var endDate = new Date( CUR_DATE.getFullYear(),  CUR_DATE.getMonth() + 1, 1, 0, 0, 0 ) ;
      createCL( db, csName, clName, '{"ShardingKey":{"TimeStamp":1}, "ShardingType":"range", "IsMainCL":true }' ) ;
      createCL( db, csName, subCLName, '{"ShardingKey":{"TimeStamp":1}, "ShardingType":"hash"}' ) ;
      eval( 'db.'+csName+'.'+clName+'.attachCL("'+csName+'.'+subCLName+'",{"LowBound":{"TimeStamp":'+beginDate.getTime()+'},"UpBound":{"TimeStamp":'+endDate.getTime()+'}})') ;
   }
   catch ( e )
   {
      if ( e == -235 )
      {
         return 0 ;
      }
      var exp = new SdbError( e, "Failed to build main cl!" ) ;
      logger.log( PDERROR, exp ) ;
      throw exp ;
   }
}

function initDatabase( db )
{
   try
   {
      createDomain( db, DM_CMBC_STAT ) ;
      createCS( db, CS_CMBC_STAT, {Domain:DM_CMBC_STAT} ) ;
      // create table for snaphost
      for ( var i = 0 ; i < SNAPSHOT_TYPE_MAX ; i++  )
      {
         var clName = ""
         clName = getTable( OP_TYPE_GET_SNAPSHOT, i ) ;
         if ( typeof(clName) == "string" && clName != "" )
         {
            createMainCL( db, CS_CMBC_STAT, clName ) ;
         }
      }
      // create table for list
      for ( var i = 0 ; i < LIST_TYPE_MAX ; i++  )
      {
         var clName = ""
         clName = getTable( OP_TYPE_GET_LIST, i ) ;
         if ( typeof(clName) == "string" && clName != "" )
         {
            createMainCL( db, CS_CMBC_STAT, clName ) ;
         }
      }
      // create table for operation log info
      createMainCL( db, CS_CMBC_STAT, CL_LOG_INFO ) ;
   }
   catch ( e )
   {
      logger.log( PDERROR, "Failed to init database!" + e ) ;
      return -1 ;
   }
   return 0 ;
}

function main()
{
   if ( typeof(COORD_LIST) != "object" )
   {
      println( "Input error: COORD_LIST" ) ;
      return -1 ;
   }

   var db = getdb() ;
   initDatabase( db ) ;

   var nodeList = getCataDataAddrList( db ) ;
   // get snapshot info
   for( var i = 0 ; i < SNAPSHOT_TYPE_MAX ; ++i )
   {
      var clName = getTable( OP_TYPE_GET_SNAPSHOT, i ) ;
      if ( typeof(clName) == "string" && clName != "" )
      {
         processSnapshot( db, i, nodeList, CUR_DATE.getTime() ) ;
      }
   }
   // get list info
   for( var i = 0 ; i < LIST_TYPE_MAX ; ++i )
   {
      var clName = getTable( OP_TYPE_GET_LIST, i ) ;
      if ( typeof(clName) == "string" && clName != "" )
      {
         processList( db, i, CUR_DATE.getTime() ) ;
      }
   }
}

var LogInfo = function()
{
   this.Operation                      = null ;
   this.OperationType                  = null ;
   this.LogLevel                       = PDINFO ;
   this.ErrNodes                       = [] ;
   this.ErrNo                          = SDB_OK ;
   this.TimeStamp                      = null ;
} ;

var logger = new Logger( "collectSnapshotInfo.js" ) ;
var DM_CMBC_STAT="DM_CMBC_STAT" ;
var CS_CMBC_STAT="CS_CMBC_STAT" ;
var CL_LOG_INFO="CL_LOG_INFO" ;
var OP_TYPE_GET_SNAPSHOT = 0 ;
var OP_TYPE_GET_LIST = 1 ;
var SNAPSHOT_TYPE_MAX=15 ;
var LIST_TYPE_MAX=15 ;
var RG_LIST ;
var CUR_DATE = new Date() ;
var IGNORE_FIELDS = [ "NodeID" ] ;



main() ;

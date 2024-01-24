import( "../lib/main.js" );
var lobdSize = 128 * 1024 * 1024;
var lobmSize = 83984384;

/*******************************************************************************
@Description : 校验集合快照信息
@Modify list : 2019-11-18 zhao xiaoni init
*******************************************************************************/
function checkStatistics ( actStatistics, expStatistics )
{
   for( var i = 0; i < expStatistics.length; i++ )
   {
      for( var j = 0; j < actStatistics.length; j++ )
      {
         if( expStatistics[i].NodeName === actStatistics[j].NodeName )
         {
            for( var key in expStatistics[j] )
            {
               assert.equal( actStatistics[j][key], expStatistics[i][key],
                  "Filed " + key + "is not equal, expStatistics = " +
                  JSON.stringify( expStatistics[i], "", 1 ) + ",actStatistics = " +
                  JSON.stringify( actStatistics[j], "", 1 ) );
            }
            break;
         }
      }
   }
}

/*******************************************************************************
@Description : 获取指定节点的集合快照信息
@Modify list : 2019-11-18 zhao xiaoni init
*******************************************************************************/
function getStatistics ( fullName, nodeNames )
{
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONS, { Name: fullName } );
   var tmpArray = [];
   var details = cursor.current().toObj().Details;
   for( var i = 0; i < details.length; i++ )
   {
      var group = cursor.current().toObj().Details[i].Group;
      for( var j = 0; j < nodeNames.length; j++ )
      {
         for( var k = 0; k < group.length; k++ )
         {
            if( group[k].NodeName === nodeNames[j] )
            {
               tmpArray.push( group[k] );
               break;
            }
         }
      }
   }
   return tmpArray;
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

/*******************************************************************************
@Description : 校验实际结果与预期结果
@Modify list : 2019-11-18 zhao xiaoni init
*******************************************************************************/
function checkResult ( actResult, expResult )
{
   assert.equal( actResult.length, expResult.length );
   assert.equal( actResult, expResult );
}


/************************************
*@Description: get a coord nod, a cata node, a data node
*@author:      zhaoxiaoni
*@createDate:  2019/10/21
**************************************/
function getNodeAddresses ()
{
   var cata = true;
   var data = true;
   var coord = true;
   var nodeAddresses = new Array();
   var cursor = db.listReplicaGroups();

   while( cursor.next() )
   {
      var groupObj = cursor.current().toObj();
      var groupArray = groupObj["Group"];
      for( var i = 0; i < groupArray.length; i++ )
      {
         var hostName = groupArray[i]["HostName"];
         var svcName = groupArray[i]["Service"][0]["Name"];
         var json = { "hostName": hostName, "svcName": svcName };
         var remote = new Remote( COORDHOSTNAME, 11790 );
         var cmd = remote.getCmd();
         var remoteHostName = cmd.run( "hostname" ).split( "\n" )[0];
         if( groupObj["Role"] === 1 && hostName != remoteHostName && coord == true )
         {
            nodeAddresses.push( json );
            coord = false;
            println( "coordNode is " + hostName + ":" + svcName );
         }
         else if( groupObj["Role"] === 2 && cata == true )
         {
            nodeAddresses.push( json );
            cata = false;
            println( "cataNode is " + hostName + ":" + svcName );
         }
         else if( groupObj["Role"] === 0 && data == true )
         {
            nodeAddresses.push( json );
            data = false;
            println( "dataNode is " + hostName + ":" + svcName );
         }
      }
   }
   return nodeAddresses;
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

function isNotContained ( actResult, expResult )
{
   var flag = true;
   for( var i = 0; i < expResult.length; i++ )
   {
      if( actResult.indexOf( expResult[i] ) !== -1 )
      {
         flag = false;
         break;
      }
   }
   return flag;
}

function checkParameters ( cursor, expResult )
{
   while( cursor.next() )
   {
      var object = cursor.current().toObj().Details[0];
      for( var i in expResult )
      {
         assert.equal( object[i], expResult[i] );
      }
   }
}

function getCursorResult ( cursor )
{
   var cursorResult = [];
   while( cursor.next() )
   {
      cursorResult.push( cursor.current().toObj()["Name"] );
   }
   cursor.close();
   return cursorResult;
}

/*******************************************************************************
@Description : 插入测试数据
@Modify list : 2020-05-31 wuyan init
*******************************************************************************/
function insertRecs ( dbcl, recsNum )
{
   if( typeof ( recsNum ) == "undefined" ) { recsNum = 100; }
   var doc = [];
   for( var i = 0; i < recsNum; i++ )
   {
      doc.push( { a: i, no: i, b: i, c: "test" + i } );
   }
   dbcl.insert( doc );
}

/*******************************************************************************
@Description : 直连主节点获取集合快照
@Modify list : 2020-05-31 wuyan init
*******************************************************************************/
function getCLSnapshotFromMasterNode ( groupName, clName )
{
   try
   {
      var masterNode = db.getRG( groupName ).getMaster();
      var masterdb = new Sdb( masterNode.getHostName(), masterNode.getServiceName() );
      var clSnapshotInfo = masterdb.snapshot( SDB_SNAP_COLLECTIONS, { Name: COMMCSNAME + "." + clName } ).next().toObj();
   }
   finally
   {
      if( masterdb !== undefined )
      {
         masterdb.close();
      }
   }
   return clSnapshotInfo;
}

function getCoordUrl ( sdb )
{
   var coordUrls = [];
   var rgInfo = sdb.getCoordRG().getDetail().current().toObj().Group;
   for( var i = 0; i < rgInfo.length; i++ )
   {
      var hostname = rgInfo[i].HostName;
      var svcname = rgInfo[i].Service[0].Name;
      coordUrls.push( hostname + ":" + svcname );
   }
   return coordUrls;
}

function sortBy ( field )
{
   return function( a, b )
   {
      return a[field] > b[field];
   }
}

function checkSnapshot ( cursor, expResults, range )
{
   if( typeof ( range ) == "undefined" ) { range = 0; }
   // 校验时允许存在误差的指标
   var checkRangeKeys = ["LobCapacity", "LobMetaCapacity", "MaxLobCapacity", "MaxLobCapSize", "UsedLobSpaceRatio", "LobUsageRate",
      "TotalLobReadSize", "TotalLobWriteSize", "TotalLobRead", "TotalLobWrite", "TotalLobTruncate", "TotalLobAddressing"];
   var i = 0;
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      for( var key in expResults[i] )
      {
         if( key == "TotalLobAddressing" )
         {
            // TotalLobAddressing允许在每次操作中存在2倍误差
            if( obj[key] < expResults[i][key] || obj[key] > ( expResults[i][key] + range ) )
            {
               throw new Error( "actual:" + JSON.stringify( obj[key], "", 1 ) + "\nexpected:" +
                  JSON.stringify( expResults[i][key], "", 1 ) + "\n" + key + "\n node name:" + expResults[i]["NodeName"] );
            }

         }
         else if( checkRangeKeys.indexOf( key ) >= 0 )
         {
            var absolute = Math.abs( obj[key] - expResults[i][key] );
            // 对于部分结果允许存在5%的误差
            if( absolute > ( obj[key] * 0.05 ) )
            {
               throw new Error( "actual:" + JSON.stringify( obj[key], "", 1 ) + "\nexpected:" +
                  JSON.stringify( expResults[i][key], "", 1 ) + "\n" + key + "\n node name:" + expResults[i]["NodeName"] );
            }
         }
         else
         {
            assert.equal( obj[key], expResults[i][key], "different fields:" + key + "\n node name:" + expResults[i]["NodeName"] );
         }
      }
      i++;
   }
   cursor.close();
}

function getSnapshotLobStat ( cursor )
{
   var lobStat = ["TotalLobGet", "TotalLobPut", "TotalLobDelete", "TotalLobReadSize", "TotalLobWriteSize", "LobCapacity",
      "LobMetaCapacity", "MaxLobCapacity", "MaxLobCapSize", "TotalLobs", "TotalLobPages", "TotalUsedLobSpace",
      "UsedLobSpaceRatio", "TotalLobList", "FreeLobSpace", "FreeLobSize", "TotalLobSize", "TotalValidLobSize", "LobUsageRate",
      "AvgLobSize", "TotalLobRead", "TotalLobWrite", "TotalLobTruncate", "TotalLobAddressing", "NodeName"];
   var actLobStats = [];
   while( cursor.next() )
   {
      var actLobStat = {};
      var obj = cursor.current().toObj();
      for( var i in lobStat )
      {
         actLobStat[lobStat[i]] = obj[lobStat[i]];
      }
      actLobStats.push( actLobStat );
   }
   cursor.close();
   actLobStats.sort( sortBy( "NodeName" ) );
   return actLobStats;
}

function checkSnapshotToCL ( cursor, expResults, isRawData, range )
{
   if( typeof ( range ) == "undefined" ) { range = 0; }
   // 校验结果时允许存在误差的指标
   var checkRangeKeys = ["LobCapacity", "LobMetaCapacity", "MaxLobCapacity", "MaxLobCapSize", "UsedLobSpaceRatio", "LobUsageRate",
      "TotalLobReadSize", "TotalLobWriteSize", "TotalLobRead", "TotalLobWrite", "TotalLobTruncate", "TotalLobAddressing"];
   var Details = "Details";
   var Group = "Group";

   var i = 0;
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      for( var key in expResults[i] )
      {
         if( isRawData )
         {
            if( key == "TotalLobAddressing" )
            {
               // TotalLobAddressing允许在每次操作中存在2倍误差
               if( obj[Details][0][key] < expResults[i][key] || obj[Details][0][key] > ( expResults[i][key] + range ) )
               {
                  throw new Error( "actual:" + JSON.stringify( obj[Details][0][key], "", 1 ) + "\nexpected:" +
                     JSON.stringify( expResults[i][key], "", 1 ) + "\n" + key + "\n node name:" + expResults[i]["NodeName"] );
               }
            }
            else if( checkRangeKeys.indexOf( key ) >= 0 )
            {
               var absolute = Math.abs( obj[Details][0][key] - expResults[i][key] );
               // 对于部分结果允许存在5%的误差
               if( absolute > ( obj[Details][0][key] * 0.05 ) )
               {
                  throw new Error( "actual:" + JSON.stringify( obj[Details][0][key], "", 1 ) + "\nexpected:" +
                     JSON.stringify( expResults[i][key], "", 1 ) + "\n" + key + "\n node name:" + expResults[i]["NodeName"] );
               }
            }
            else
            {
               assert.equal( obj[Details][0][key], expResults[i][key], "different fields:" + key + "\n node name:" + expResults[i]["NodeName"] );
            }
         }
         else
         {
            var objInfo = obj[Details][0][Group].sort( sortBy( "NodeName" ) );
            for( var j in obj[Details][0][Group] )
            {
               if( key == "TotalLobAddressing" )
               {
                  // TotalLobAddressing允许在每次操作中存在2倍误差
                  if( objInfo[j][key] < expResults[j][key] || objInfo[j][key] > ( expResults[j][key] + range ) )
                  {
                     throw new Error( "actual:" + JSON.stringify( obj[Details][0][Group][j][key], "", 1 ) + "\nexpected:" +
                        JSON.stringify( expResults[j][key], "", 1 ) + "\n" + key + "\n node name:" + expResults[j]["NodeName"] +
                        " act node : " + objInfo[j]["NodeName"] );
                  }
               }
               else if( checkRangeKeys.indexOf( key ) >= 0 )
               {
                  var absolute = Math.abs( objInfo[j][key] - expResults[j][key] );
                  // 对于部分结果允许存在5%的误差
                  if( absolute > ( objInfo[j][key] * 0.05 ) )
                  {
                     throw new Error( "actual:" + JSON.stringify( obj[Details][0][Group][j][key], "", 1 ) + "\nexpected:" +
                        JSON.stringify( expResults[j][key], "", 1 ) + "\n" + key + "\n node name:" + expResults[j]["NodeName"] +
                        " act node : " + objInfo[j]["NodeName"] );
                  }
               }
               else
               {
                  assert.equal( obj[Details][0][Group][j][key], expResults[j][key], "different fields:" + key + "\n node name:" +
                     expResults[j]["NodeName"] + " act node : " + objInfo[j]["NodeName"] );
               }
            }
         }
      }
      i++;
   }
   cursor.close();
}

function getSnapshotLobStatToCL ( cursor, isRawData )
{
   var Details = "Details";
   var Group = "Group";
   var lobStat = ["TotalLobGet", "TotalLobPut", "TotalLobDelete", "TotalLobReadSize", "TotalLobWriteSize", "LobCapacity",
      "LobMetaCapacity", "MaxLobCapacity", "MaxLobCapSize", "TotalLobs", "TotalLobPages", "TotalUsedLobSpace",
      "UsedLobSpaceRatio", "TotalLobList", "FreeLobSpace", "FreeLobSize", "TotalLobSize", "TotalValidLobSize", "LobUsageRate",
      "AvgLobSize", "TotalLobRead", "TotalLobWrite", "TotalLobTruncate", "TotalLobAddressing", "NodeName"];
   var actLobStats = [];

   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      if( isRawData )
      {
         var actLobStat = {};
         for( var i in lobStat )
         {
            actLobStat[lobStat[i]] = obj[Details][0][lobStat[i]];
         }
         actLobStats.push( actLobStat );
      }
      else
      {
         for( j in obj[Details][0][Group] )
         {
            var actLobStat = {};
            for( var i in lobStat )
            {
               actLobStat[lobStat[i]] = obj[Details][0][Group][j][lobStat[i]];
            }
            actLobStats.push( actLobStat );
         }
      }
   }
   cursor.close()
   actLobStats.sort( sortBy( "NodeName" ) );
   return actLobStats;
}

function getSnapshotResults ( cursor, fields )
{
   if( fields == undefined ) { fields = ["TotalTransCommit", "TotalTransRollback", "NodeName"]; }
   var results = [];
   while( cursor.next() )
   {
      var result = {};
      var obj = cursor.current().toObj();
      for( var i in fields )
      {
         result[fields[i]] = obj[fields[i]];
      }
      results.push( result );
   }
   cursor.close();
   results.sort( sortBy( "NodeName" ) );
   return results;
}

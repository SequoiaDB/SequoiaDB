import( "../lib/basic_operation/commlib.js" );
import( "../lib/main.js" );

/************************************
*@Description: update data
*@author:      wuyan
*@createDate:  2017.7.20
**************************************/
function updateData ( dbcl, updateCondition, findCondition, hint, ShardingKeyFlag )
{
   if( typeof ( findCondition ) == "undefined" ) { findCondition = {}; }
   if( typeof ( hint ) == "undefined" ) { hint = {}; }
   if( typeof ( ShardingKeyFlag ) == "undefined" ) { ShardingKeyFlag = ""; }

   if( "" !== ShardingKeyFlag )
   {
      dbcl.update( updateCondition, findCondition, hint, { KeepShardingKey: ShardingKeyFlag } );
   }
   else
   {
      dbcl.update( updateCondition, findCondition, hint );
   }
}

/************************************
*@Description: upsert data
*@author:      wuyan
*@createDate:  2017.7.20
**************************************/
function upsertData ( dbcl, upsertCondition, findCondition, hint, setOnInsert, ShardingKeyFlag )
{
   if( typeof ( findCondition ) == "undefined" ) { findCondition = {}; }
   if( typeof ( setOnInsert ) == "undefined" ) { setOnInsert = {}; }
   if( typeof ( hint ) == "undefined" ) { hint = {}; }
   if( typeof ( ShardingKeyFlag ) == "undefined" ) { ShardingKeyFlag = ""; }
   dbcl.upsert( upsertCondition, findCondition, hint, setOnInsert, { KeepShardingKey: ShardingKeyFlag } );
}


/************************************
*@Description: update data error,include update/findAndUpdate
*@author:      wuyan
*@createDate:  2017.8.22
**************************************/
function updateDataError ( dbcl, operation, updateCondition, findCondition, hint )
{
   if( typeof ( findCondition ) == "undefined" ) { findCondition = {}; }
   if( typeof ( hint ) == "undefined" ) { hint = {}; }

   try
   {
      if( operation === "update" )
      {
         dbcl.update( updateCondition, findCondition, hint, { KeepShardingKey: true } );
      }
      else if( operation === "findAndUpdate" )
      {
         dbcl.find( findCondition ).update( updateCondition, true, { KeepShardingKey: true } ).toArray();
      }
      else if( operation === "upsert" )
      {
         dbcl.upsert( updateCondition, findCondition, hint, null, { KeepShardingKey: true } );
      }
      throw new Error( "---update shardingKey should be wrong" );
   }
   catch( e )
   {
      if( SDB_UPDATE_SHARD_KEY != e.message )
      {
         throw e;
      }
   }
}

/************************************
*@Description: find and update data
*@author:      wuyan
*@createDate:  2017.7.20
**************************************/
function findAndUpdateData ( dbcl, findCondition, updateCondition, returnFlag, ShardingKeyFlag )
{
   if( typeof ( findCondition ) == "undefined" ) { findCondition = {}; }
   if( typeof ( returnFlag ) == "undefined" ) { returnFlag = false; }
   if( typeof ( ShardingKeyFlag ) == "undefined" ) { ShardingKeyFlag = { KeepShardingKey: false }; }

   if( true == ShardingKeyFlag )
   {
      dbcl.find( findCondition ).update( updateCondition, returnFlag, { KeepShardingKey: true } ).toArray();
   }
   else
   {
      dbcl.find( findCondition ).update( updateCondition, returnFlag, { KeepShardingKey: false } ).toArray();
   }
}

/************************************
*@Description: get actual result and check it
**************************************/
function checkResult ( dbcl, findCondition, findCondition2, expRecs )
{
   if( typeof ( findCondition ) == "undefined" ) { findCondition = null; }
   var rc = dbcl.find( findCondition, findCondition2 ).sort( { _id: 1 } );
   checkRec( rc, expRecs );
}

/****************************************************
@description: check the result of query
@modify list:
              2016-3-3 yan WU init
****************************************************/
function checkRec ( rc, expRecs )
{
   //get actual records to array
   var actRecs = [];
   while( rc.next() )
   {
      actRecs.push( rc.current().toObj() );
   }

   //check count
   assert.equal( actRecs.length, expRecs.length );

   //check every records every fields,expRecs as compare source
   for( var i in expRecs )
   {
      var actRec = actRecs[i];
      var expRec = expRecs[i];
      for( var f in expRec )
      {
         assert.equal( actRec[f], expRec[f] );
      }
   }

   //check every records every fields,actRecs as compare source
   for( var i in actRecs )
   {
      var actRec = actRecs[i];
      var expRec = expRecs[i];

      for( var f in actRec )
      {
         if( f == "_id" )
         {
            continue;
         }
         assert.equal( actRec[f], expRec[f] );
      }
   }
}

/************************************
*@Description: get group name and service name .
*@author:      wuyan
*@createDate:  2015/10/20
**************************************/
function getGroupName ( db, mustBePrimary )
{
   var RGname = null;
   RGname = db.listReplicaGroups().toArray();
   var j = 0;
   var arrGroupName = Array();
   for( var i = 1; i != RGname.length; ++i )
   {
      var eRGname = eval( '(' + RGname[i] + ')' );
      if( 1000 <= eRGname["GroupID"] )
      {
         arrGroupName[j] = Array();
         var primaryNodeID = eRGname["PrimaryNode"];
         var groups = eRGname["Group"];
         for( var m = 0; m < groups.length; m++ )
         {
            if( true == mustBePrimary )
            {
               var nodeID = groups[m]["NodeID"];
               if( primaryNodeID != nodeID )
                  continue;
            }
            arrGroupName[j].push( eRGname["GroupName"] );
            arrGroupName[j].push( groups[m]["HostName"] );
            arrGroupName[j].push( groups[m]["Service"][0]["Name"] );
            break;
         }
         ++j;
      }
   }
   return arrGroupName;
}

/************************************
*@Description: create maincl .
*@author:      wuyan
*@createDate:  2015/10/20
**************************************/
function createMainCL ( csName, mainCLName, shardingKey )
{

   var options = { ShardingKey: shardingKey, IsMainCL: true, ReplSize: 0 };
   var mainCL = commCreateCL( db, csName, mainCLName, options, false,
      true, "Failed to create mainCL." );
   return mainCL;
}

function createCL ( csName, clName, shardingKey, shardingType )
{
   if( typeof ( shardingType ) == "undefined" ) { shardingType = "hash"; }

   var options = { ShardingKey: shardingKey, ShardingType: shardingType, ReplSize: 0, Compressed: true };
   var dbcl = commCreateCL( db, csName, clName, options, true,
      true, "Failed to create cl." );
   return dbcl;
}

/************************************
*@Description: get the informations of the srcGroups and targetGroups,then split cl with different options,
               only split 1 times
               return the informations of the srcGroups and targetGroups
*@author��Yan Wu 2015/10/26
*@parameters:
             startCondition:start condition of split,if the typeof is number,then percentage split,if the typeof is object,
             then range split
             endCondition:object of end condition
*@return array[][] ex:
        [0]
           {"GroupName":"XXXX"}
           {"HostName":"XXXX"}
           {"svcname":"XXXX"}
        [N]
**************************************/
function clSplit ( csName, clName, startCondition, endCondition )
{
   var targetGroupNums = 1;
   var groupsInfo = getSplitGroups( csName, clName, targetGroupNums );
   var srcGrName = groupsInfo[0].GroupName;
   var tarGrName = groupsInfo[1].GroupName;
   var CL = db.getCS( csName ).getCL( clName );
   if( typeof ( startCondition ) === "number" ) //percentage split
   {
      CL.split( srcGrName, tarGrName, startCondition );
   }
   else if( typeof ( startCondition ) === "object" && endCondition === undefined ) //range split without end condition
   {
      CL.split( srcGrName, tarGrName, startCondition );
   }
   else if( typeof ( startCondition ) === "object" && typeof ( endCondition ) === "object" ) //range split with end condition
   {
      CL.split( srcGrName, tarGrName, startCondition, endCondition );
   }

   return groupsInfo;
}

/************************************
*@Description: get SrcGroup and TargetGroup info,the groups information
               include GroupName,HostName and svcname
*@author��wuyan 2015/10/14
*@return array[][] ex:
        [0]
           {"GroupName":"XXXX"}
           {"HostName":"XXXX"}
           {"svcname":"XXXX"}
        [N]
           ...
**************************************/
function getSplitGroups ( csName, clName, targetGrMaxNums )
{
   var allGroupInfo = getGroupName( db, true );
   var srcGroupName = getSrcGroup( csName, clName );
   var splitGroups = new Array();
   if( targetGrMaxNums >= allGroupInfo.length - 1 )
   {
      targetGrMaxNums = allGroupInfo.length - 1;
   }
   var index = 1;

   for( var i = 0; i != allGroupInfo.length; ++i )
   {
      if( srcGroupName == allGroupInfo[i][0] )
      {
         splitGroups[0] = new Object();
         splitGroups[0].GroupName = allGroupInfo[i][0];
         splitGroups[0].HostName = allGroupInfo[i][1];
         splitGroups[0].svcname = allGroupInfo[i][2];
      }
      else
      {
         if( index > targetGrMaxNums )
         {
            continue;
         }
         splitGroups[index] = new Object();
         splitGroups[index].GroupName = allGroupInfo[i][0];
         splitGroups[index].HostName = allGroupInfo[i][1];
         splitGroups[index].svcname = allGroupInfo[i][2];
         index++;
      }
   }
   return splitGroups;

}

/************************************
*@Description: get SrcGroup name,update getPG to getSrcGroup
*@author��wuyan 2015/10/14
**************************************/
function getSrcGroup ( csName, clName )
{
   var clFullName = csName + "." + clName;
   var clInfo = db.snapshot( 8, { Name: clFullName } );
   while( clInfo.next() )
   {
      var clInfoObj = clInfo.current().toObj();
      var srcGroupName = clInfoObj.CataInfo[0].GroupName;
   }
   return srcGroupName;
}

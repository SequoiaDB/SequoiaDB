
import( "../lib/main.js" );
import( "../lib/basic_operation/commlib.js" );

/************************************
*@Description: aggregate data and check result
*@author:      zhaoyu
*@createDate:  2015.5.20
**************************************/
function aggregateCheckResult ( cl, cond, expRecs )
{
   var cursor = cl.aggregate( cond );
   commCompareResults( cursor, expRecs );
}

/************************************
*@Description: find data use sql,get actual result and check it 
*@author:      zhaoyu
*@createDate:  2015.5.20
**************************************/
function checkSqlResult ( db, sql, expRecs )
{
   var cursor = db.exec( sql );
   commCompareResults( cursor, expRecs );
}

/************************************
*@Description: get actual result and check it 
*@author:      zhaoyu
*@createDate:  2015.5.20
**************************************/
function checkResult ( cl, cond, sel, expRecs, sort )
{
   if( typeof ( cond ) === "undefined" ) { cond = {}; }
   if( typeof ( sel ) === "undefined" ) { sel = {}; }
   if( typeof ( expRecs ) === "undefined" ) { expRecs = sel; }
   var cursor = cl.find( cond, sel ).sort( sort );
   // when expRecs is JSON
   if( expRecs == sel )
   {
      var expRecsArr = [];
      expRecsArr.push( expRecs );
   }
   // when expRecs is Arr
   else
   {
      var expRecsArr = expRecs;
   }

   commCompareResults( cursor, expRecsArr );
}

/************************************
*@Description: get the informations of the srcGroups and targetGroups,then split cl with different options,
               only split 1 times
               return the informations of the srcGroups and targetGroups
*@author:      wuyan
*@createdate:  2015.10.14
**************************************/
function ClSplitOneTimes ( csName, clName, startCondition, endCondition )
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
*@Description: get Group name and Service name
*@author：wuyan 2015/10/20
**************************************/
function getGroupName ( db, mustBePrimary )
{
   var RGname = db.listReplicaGroups().toArray();
   var j = 0;
   var arrGroupName = new Array();
   for( var i = 1; i != RGname.length; ++i )
   {
      var eRGname = eval( '(' + RGname[i] + ')' );
      if( 1000 <= eRGname["GroupID"] )
      {
         arrGroupName[j] = new Array();
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
*@Description: get SrcGroup and TargetGroup info,the groups information
               include GroupName,HostName and svcname
*@author:      wuyan
*@createdate:  2015.10.14
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
*@author:      wuyan
*@createdate:  2015.10.14
**************************************/
function getSrcGroup ( csName, clName )
{
   if( undefined == csName || undefined == clName )
   {
      throw new Error( "csName: " + csName + "\nclName: " + clName );
   }
   var tableName = csName + "." + clName;
   var cataMaster = db.getCatalogRG().getMaster().toString().split( ":" );
   var catadb = new Sdb( cataMaster[0], cataMaster[1] );
   var Group = catadb.SYSCAT.SYSCOLLECTIONS.find().toArray();
   var srcGroupName;
   for( var i = 0; i < Group.length; ++i )
   {
      var eachID = eval( "(" + Group[i] + ")" );
      if( tableName == eachID["Name"] )
      {
         srcGroupName = eachID["CataInfo"][0]["GroupName"];
         break;
      }
   }
   return srcGroupName;
}

/************************************
*@Description: get the actual result from src group and des group,
               get the expect result from the function argument,
               then check result.
*@author:      zhaoyu 
*@createDate:  2016/4/28
*@parameters:  dataNodeInfo:[object1,object2,.....]
               expRecs:[[object1,object2,.....],[object3,object4,.....]]               
**************************************/
function checkRangeClSplitResult ( db, clName, dataNodeInfo, cond, sel, expRecs, sort )
{
   //get data from src and des groups
   for( var i = 0; i < dataNodeInfo.length; i++ )
   {
      try
      {
         //get data from function argument
         var expGroupRecs = expRecs[i];

         var db1 = new Sdb( dataNodeInfo[i].HostName, dataNodeInfo[i].svcname );
         var cl = db1.getCS( COMMCSNAME ).getCL( clName );
         var cursor = cl.find( cond, sel ).sort( sort );

         //check data
         commCompareResults( cursor, expGroupRecs );
      }
      finally
      {
         if( db1 !== undefined )
         {
            db1.close();
         }
      }
   }
}

function checkHashClSplitResult ( db, clName, dataNodeInfo, cond, sel, expRecs, sort )
{
   //get data from src and dst groups
   var actRecs = new Array();
   for( var i = 0; i < dataNodeInfo.length; i++ )
   {
      try
      {
         //get data from master node
         var db1 = new Sdb( dataNodeInfo[i].HostName, dataNodeInfo[i].svcname );
         var cl = db1.getCS( COMMCSNAME ).getCL( clName );
         var cursor = cl.find( cond, sel ).sort( sort );

         //get actual records to array
         actRecs[i] = new Array();
         while( cursor.next() )
         {
            actRecs[i].push( cursor.current().toObj() );
         }
      }
      finally
      {
         if( db1 !== undefined )
         {
            db1.close();
         }
      }
   }

   //check count
   var arrLength = 0;
   for( var j = 0; j < actRecs.length; j++ )
   {
      arrLength += actRecs[j].length;
   }
   if( arrLength !== expRecs )
   {
      throw new Error( "arrLength: " + arrLength + "\nexpRecs: " + expRecs );
   }
   //check every records every fields
   for( var s in actRecs[0] )
   {
      for( var x in actRecs[1] )
      {
         if( actRecs[0][s] == actRecs[1][x] )
         {
            throw new Error( "actRecs[0]: " + actRecs[0] + "\nactRecs[1]: " + actRecs[1] );
         }
      }
   }
}

/************************************
*@Description: check result when the expect result of find data is failed.
*@author:      zhaoyu 
*@createDate:  2016/4/28
*@parameters:               
**************************************/
function InvalidArgCheck ( cl, cond, sel, errno )
{
   try
   {
      cl.find( cond, sel ).toArray();
      throw new Error( "need throw error" );
   }
   catch( e )
   {
      if( errno != e.message )
      {
         throw e;
      }
   }
}

/************************************
*@Description: check result when the expect result of insert data is failed.
*@author:      zhaoyu 
*@createDate:  2016/4/28
*@parameters:               
**************************************/
function invalidDataInsertCheckResult ( cl, invalidDoc, errno )
{
   try
   {
      cl.insert( invalidDoc );
      throw new Error( "need throw error" );
   }
   catch( e )
   {
      if( errno != e.message )
      {
         throw e;
      }
   }
}

/************************************
*@Description: check test environment before split.
*@author:      zhaoyu 
*@createDate:  2016/5/23
*@parameters:               
**************************************/
function checkConditionBeforeSplit ( db )
{
   //standalone can not split
   if( true == commIsStandalone( db ) )
   {
      return;
   }
   //less two groups,can not split
   var allGroupName = getGroupName( db );
   if( 1 === allGroupName.length )
   {
      return;
   }
}

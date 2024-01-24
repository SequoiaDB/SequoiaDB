
import( "../lib/basic_operation/commlib.js" );
import( "../lib/main.js" );

/************************************
*@Description: compare actual and expect result, 
they is not the same, return error, 
else return ok
*@author:      zhaoyu
*@createDate:  2015.5.20
**************************************/
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

   //check every records every fields
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
*@Description: get actual result and check it
*@author:      zhaoyu
*@createDate:  2017.5.18
**************************************/
function checkResult ( dbcl, findConf, hintConf, sortConf, expRecs )
{
   if( hintConf == undefined )
   {
      throw new Error( "NOT_SET_HINT" );
   }
   for( var i = 0; i < hintConf.length; i++ )
   {
      var rc = dbcl.find( findConf ).sort( sortConf ).hint( hintConf[i] );
      checkRec( rc, expRecs );
   }
}

/************************************
*@Description: check result when the expect result of find data is failed.
*@author:      zhaoyu
*@createDate:  2016/4/28
*@parameters:
**************************************/
function InvalidArgCheck ( dbcl, condition, condition2, expRecs )
{
   assert.tryThrow( expRecs, function()
   {
      dbcl.find( condition, condition2 ).toArray();
   } );
}

/************************************
*@Description: get SrcGroup and TargetGroup info, the groups information
include GroupName, HostName and svcname
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
*@Description: get the informations of the srcGroups and targetGroups, then split cl with different options, 
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
   if( typeof ( startCondition ) === "number" )//percentage split
   {
      CL.split( srcGrName, tarGrName, startCondition );
   }
   else if( typeof ( startCondition ) === "object" && endCondition === undefined )//range split without end condition
   {
      CL.split( srcGrName, tarGrName, startCondition );
   }
   else if( typeof ( startCondition ) === "object" && typeof ( endCondition ) === "object" )//range split with end condition
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
   var RGname = null;
   RGname = db.listReplicaGroups().toArray();
   var j = 0;
   var arrGroupName = Array();
   for( var i = 1; i != RGname.length; ++i )
   {
      var eRGname = eval( '( ' + RGname[i] + ' )' );
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
*@Description: get SrcGroup name, update getPG to getSrcGroup
*@author:      wuyan
*@createdate:  2015.10.14
**************************************/
function getSrcGroup ( csName, clName )
{
   if( undefined == csName || undefined == clName )
   {
      throw new Error( "cs or cl name is undefined" );
   }
   var tableName = csName + "." + clName;
   var cataMaster = db.getCatalogRG().getMaster().toString().split( ":" );
   var catadb = new Sdb( cataMaster[0], cataMaster[1] );
   var Group = catadb.SYSCAT.SYSCOLLECTIONS.find().toArray();
   var srcGroupName;
   for( var i = 0; i < Group.length; ++i )
   {
      var eachID = eval( "( " + Group[i] + " )" );
      if( tableName == eachID["Name"] )
      {
         srcGroupName = eachID["CataInfo"][0]["GroupName"];
         break;
      }
   }
   return srcGroupName;

}

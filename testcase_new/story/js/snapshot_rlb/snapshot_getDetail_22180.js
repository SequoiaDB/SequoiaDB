/******************************************************************************
*@Description : seqDB-22180:设置会话访问属性，通过getDetail()获取集合快照信息֤
*@author:      wuyan
*@createdate:  2020.05.11
******************************************************************************/
testConf.skipStandAlone = true;

var clName = COMMCLNAME + "_snapshot_cl_22180";
main( test );

function test ( testPara )
{
   var nodeNum = 3;
   var groups = getGroupsWithNodeNum( nodeNum );
   if( groups.length - 1 < nodeNum )
   {
      return;
   }

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );
   var groupName = groups[0].GroupName;
   var dbcl = commCreateCL( db, COMMCSNAME, clName, { Group: groupName } );
   try
   {
      var useInstanceid = 18;
      var instanceid = [10, 5, 18];
      for( var i = 0; i < instanceid.length; i++ )
      {
         var hostName = groups[i + 1].HostName;
         var svcName = groups[i + 1].svcname;

         if( instanceid[i] == useInstanceid )
         {
            var expHostName = hostName;
            var expSvcName = svcName;
         }
         updateConf( db, { instanceid: instanceid[i] }, { NodeName: hostName + ":" + svcName } );
      }
      db.getRG( groupName ).stop();
      db.getRG( groupName ).start();
      commCheckBusinessStatus( db );
      db.invalidateCache();

      insertRecs( dbcl );
      commCheckBusinessStatus( db );

      accessNodeWithInstanceId( dbcl, groupName, expHostName, expSvcName, useInstanceid, clName )
      accessNodeWithSlave( dbcl, groupName, clName );
      commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the ending" );
   }
   finally
   {
      for( var i = 0; i < instanceid.length; i++ )
      {
         var hostName = groups[i + 1].HostName;
         var svcName = groups[i + 1].svcname;
         deleteConf( db, { instanceid: 1 }, { NodeName: hostName + ":" + svcName } );
      }
      db.getRG( groupName ).getNode( hostName, svcName ).stop();
      db.getRG( groupName ).getNode( hostName, svcName ).start();
      commCheckBusinessStatus( db );
   }
}

function accessNodeWithInstanceId ( dbcl, groupName, hostName, svcName, useInstanceid, clName )
{

   var clSnapshot = getCLSnapshotFromSetNode( hostName, svcName, clName );
   db.setSessionAttr( { PreferedInstance: useInstanceid } );
   var getDetailInfo = dbcl.getDetail().next().toObj();

   //检查访问节点为指定对应的节点
   var nodeName = hostName + ":" + svcName;
   if( getDetailInfo.Details[0]["NodeName"] !== nodeName )
   {
      throw new Error( "\nExpAccess instanceid node is " + nodeName + "\n" + "\n but actAccess node is "
         + getDetailInfo.Details[0]["NodeName"] );
   }

   //由于时间不一致，剔除"ResetTimestamp"字段后比较结果
   delete getDetailInfo.Details[0]["ResetTimestamp"];
   delete clSnapshot.Details[0]["ResetTimestamp"];
   checkResult( getDetailInfo, clSnapshot );
}

function accessNodeWithSlave ( dbcl, groupName, clName )
{
   db.setSessionAttr( { PreferedInstance: "S" } );
   var getDetailInfo = dbcl.getDetail().next().toObj();
   var accessNodeName = getDetailInfo.Details[0]["NodeName"];
   var nodeStr = accessNodeName.split( ':' );
   if( nodeStr.length !== 2 )
   {
      throw new Error( e );
   }
   var accessHostName = nodeStr[0];
   var accessSvcName = nodeStr[1];
   var clSnapshot = getCLSnapshotFromSetNode( accessHostName, accessSvcName, clName );

   //检查访问节点为备节点
   var primaryNode = db.getRG( groupName ).getMaster();
   if( accessNodeName == primaryNode )
   {
      throw new Error( "\naccess node is " + accessNodeName + "\n" + "\n primaryNode is "
         + primaryNode );
   }

   delete getDetailInfo.Details[0]["ResetTimestamp"];
   delete clSnapshot.Details[0]["ResetTimestamp"];
   checkResult( getDetailInfo, clSnapshot );
}

function getCLSnapshotFromSetNode ( hostName, svcName, clName )
{
   try
   {
      var sdb = new Sdb( hostName, svcName );
      var clSnapshotInfo = sdb.snapshot( SDB_SNAP_COLLECTIONS, { Name: COMMCSNAME + "." + clName } ).next().toObj();
   }
   finally
   {
      if( sdb !== undefined )
         sdb.close();
   }
   return clSnapshotInfo;
}

function getGroupsWithNodeNum ( nodesNum )
{
   var groupArray = commGetGroups( db );
   var groupInfo = [];
   for( var i = 0; i < groupArray.length; i++ )
   {
      groupInfo = groupArray[i];
      if( groupInfo.length - 1 >= nodesNum )
      {
         return groupInfo;
      }
   }
   return groupInfo;
}

function updateConf ( db, configs, options )
{
   try
   {
      db.updateConf( configs, options );
   }
   catch( e )
   {
      if( e != SDB_COORD_NOT_ALL_DONE && e != SDB_RTN_CONF_NOT_TAKE_EFFECT )
      {
         throw new Error( e );
      }
   }
}

function deleteConf ( db, configs, options )
{
   try
   {
      db.deleteConf( configs, options );
   }
   catch( e )
   {
      if( e != SDB_COORD_NOT_ALL_DONE && e != SDB_RTN_CONF_NOT_TAKE_EFFECT )
      {
         throw new Error( e );
      }
   }
}

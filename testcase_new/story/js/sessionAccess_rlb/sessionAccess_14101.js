/* *****************************************************************************
@description: seqDB-14101:多组查询，设置会话访问属性，preferedinstance值为其他组未设置instanceid的节点下标
@author: 2018-1-24 wuyan  Init
***************************************************************************** */
testConf.skipStandAlone = true;
testConf.skipOneDuplicatePerGroup = true;

main( test );

function test ()
{
   var groups = getGroupsWithNodeNum( 3 );
   if( groups.length === 0 )
   {
      return;
   }
   var group1 = groups[0].sort( sortBy( "NodeID" ) );
   var group2 = groups[1].sort( sortBy( "NodeID" ) );
   var groupName1 = group1[0].GroupName;
   var groupName2 = group2[0].GroupName;
   var clName = CHANGEDPREFIX + "_14101";
   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName, { Group: groupName1, ShardingKey: { no: 1 } } );
   insertData( cl );
   cl.split( groupName1, groupName2, 50 );

   var instanceid = 2;
   var hostName = group1[1].HostName;
   var svcName = group1[1].svcname;
   var expAccessNodes = [hostName + ":" + svcName];
   updateConf( db, { instanceid: instanceid }, { NodeName: hostName + ":" + svcName }, -322 );
   db.getRG( groupName1 ).getNode( hostName, svcName ).stop();
   db.getRG( groupName1 ).getNode( hostName, svcName ).start();
   try
   {
      commCheckBusinessStatus( db );
      db.invalidateCache();

      var options = { PreferedInstance: instanceid };
      expAccessNodes.push( group1[2].HostName + ":" + group1[2].svcname );
      expAccessNodes.push( group2[2].HostName + ":" + group2[2].svcname );
      checkAccessNodes( cl, expAccessNodes, options );

      commDropCL( db, COMMCSNAME, clName, false, false );
   }
   finally
   {
      deleteConf( db, { instanceid: 1 }, { NodeName: hostName + ":" + svcName }, -322 );
      db.getRG( groupName1 ).getNode( hostName, svcName ).stop();
      db.getRG( groupName1 ).getNode( hostName, svcName ).start();
      commCheckBusinessStatus( db );
   }
}


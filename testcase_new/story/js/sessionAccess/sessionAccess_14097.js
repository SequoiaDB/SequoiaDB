/* *****************************************************************************
@description: seqDB-14097:多组查询，设置会话访问属性，preferedinstance为多个组中节点的instanceid
@author: 2018-1-29 wuyan  Init
***************************************************************************** */
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;

main( test );

function test ()
{
   var groups = getGroupsWithNodeNum( 2 );
   if( groups.length === 0 )
   {
      return;
   }
   var group1 = groups[0].sort( sortBy( "NodeID" ) );
   var group2 = groups[1].sort( sortBy( "NodeID" ) );
   var groupName1 = group1[0].GroupName;
   var groupName2 = group2[0].GroupName;
   var clName = CHANGEDPREFIX + "_14097";

   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName, { ShardingKey: { no: 1 }, Group: groupName1, ReplSize: 0 } );
   insertData( cl );
   cl.split( groupName1, groupName2, 50 );
   var options = { PreferedInstance: 1 };
   var expAccessNodes = [group1[1].HostName + ":" + group1[1].svcname, group2[1].HostName + ":" + group2[1].svcname];
   checkAccessNodes( cl, expAccessNodes, options );

   commDropCL( db, COMMCSNAME, clName, false, false );
}


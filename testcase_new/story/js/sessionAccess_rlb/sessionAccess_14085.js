/* *****************************************************************************
@description: seqDB-14085:设置会话属性，preferedinstance指定instanceid与其它节点下标相同的实例 
@author: 2018-1-24 wuyan  Init
***************************************************************************** */
testConf.skipStandAlone = true;
testConf.skipOneDuplicatePerGroup = true;

main( test );

function test ()
{
   var clName = CHANGEDPREFIX + "_14085";
   var groups = commGetGroups( db );
   var group = groups[0].sort( sortBy( "NodeID" ) );
   var groupName = group[0].GroupName;
   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName, { Group: groupName } );
   insertData( cl );

   var instanceid = 2;
   var hostName = group[1].HostName;
   var svcName = group[1].svcname;
   var expAccessNodes = [hostName + ":" + svcName];
   updateConf( db, { instanceid: instanceid }, { NodeName: hostName + ":" + svcName }, -322 );
   db.getRG( groupName ).getNode( hostName, svcName ).stop();
   db.getRG( groupName ).getNode( hostName, svcName ).start();
   try
   {
      commCheckBusinessStatus( db );
      db.invalidateCache();
      expAccessNodes.push( group[2].HostName + ":" + group[2].svcname );
      var options = { PreferedInstance: instanceid };
      checkAccessNodes( cl, expAccessNodes, options );
   }
   finally
   {
      deleteConf( db, { instanceid: 1 }, { NodeName: hostName + ":" + svcName }, -322 );
      db.getRG( groupName ).getNode( hostName, svcName ).stop();
      db.getRG( groupName ).getNode( hostName, svcName ).start();
      commCheckBusinessStatus( db );
   }
   commDropCL( db, COMMCSNAME, clName, false, false );
}


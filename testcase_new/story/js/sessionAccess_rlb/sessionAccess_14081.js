/* *****************************************************************************
@description: seqDB-14081:设置会话访问属性，单值指定preferedinstance存在的实例 
@author: 2018-1-22 wuyan  Init
***************************************************************************** */
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var clName = CHANGEDPREFIX + "_14081";
   var groups = commGetGroups( db );
   var group = groups[0];
   var groupName = group[0].GroupName;
   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName, { Group: groupName } );
   insertData( cl );

   var hostName = group[1].HostName;
   var svcName = group[1].svcname;
   var instanceid = 4;
   updateConf( db, { instanceid: instanceid }, { NodeName: hostName + ":" + svcName }, -322 );
   db.getRG( groupName ).getNode( hostName, svcName ).stop();
   db.getRG( groupName ).getNode( hostName, svcName ).start();
   try
   {
      commCheckBusinessStatus( db );
      db.invalidateCache();
      var options = { PreferedInstance: instanceid };
      var expAccessNodes = [hostName + ":" + svcName];
      checkAccessNodes( cl, expAccessNodes, options );

      updateConf( db, { instanceid: 11 }, { NodeName: hostName + ":" + svcName }, -322 );
      db.getRG( groupName ).getNode( hostName, svcName ).stop();
      db.getRG( groupName ).getNode( hostName, svcName ).start();
      commCheckBusinessStatus( db );
      db.invalidateCache();

      options = { PreferedInstance: [11] };
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


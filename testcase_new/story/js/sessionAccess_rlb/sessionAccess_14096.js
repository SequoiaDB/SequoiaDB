/* *****************************************************************************
@description: seqDB-14096:设置会话访问属性，指定preferedinstance包含【8/9/10】
@author: 2018-1-24 wuyan  Init
***************************************************************************** */
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var groups = getGroupsWithNodeNum( 3 );
   if( groups.length === 0 )
   {
      return;
   }
   var group = groups[0];
   var groupName = group[0].GroupName;
   var clName = CHANGEDPREFIX + "_14096";
   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName, { Group: groupName } );
   insertData( cl );
   var instanceid = [8, 9, 10];
   for( var i = 0; i < instanceid.length; i++ )
   {
      var hostName = group[i + 1].HostName;
      var svcName = group[i + 1].svcname;
      var nodeName = hostName + ":" + svcName;
      updateConf( db, { instanceid: instanceid[i] }, { NodeName: nodeName }, -322 );
   }
   db.getRG( groupName ).stop();
   db.getRG( groupName ).start();
   try
   {
      commCheckBusinessStatus( db );
      db.invalidateCache();
      var options = { PreferedInstance: 8 };
      var expAccessNodes = [group[1].HostName + ":" + group[1].svcname];
      checkAccessNodes( cl, expAccessNodes, options );

      options = { PreferedInstance: 9 };
      var expAccessNodes = [group[2].HostName + ":" + group[2].svcname];
      checkAccessNodes( cl, expAccessNodes, options );

      options = { PreferedInstance: 10 };
      var expAccessNodes = [group[3].HostName + ":" + group[3].svcname];
      checkAccessNodes( cl, expAccessNodes, options );

      commDropCL( db, COMMCSNAME, clName, false, false );
   }
   finally
   {
      deleteConf( db, { instanceid: 1 }, { groupName: groupName }, -322 );
      db.getRG( groupName ).stop();
      db.getRG( groupName ).start();
      commCheckBusinessStatus( db );
   }
}

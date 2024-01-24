/******************************************************************************
@description: seqDB-14086:设置会话访问属性preferedinstance为多个不同值，preferedinstanceMode覆盖不同值
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
   var clName = CHANGEDPREFIX + "_14086";
   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName, { Group: groupName } );
   insertData( cl );

   var expAccessNodes = [];
   var instanceid = [22, 23];
   for( var i = 0; i < instanceid.length; i++ )
   {
      var hostName = group[i + 1].HostName;
      var svcName = group[i + 1].svcname;
      var nodeName = hostName + ":" + svcName;
      expAccessNodes.push( nodeName );
      updateConf( db, { instanceid: instanceid[i] }, { NodeName: nodeName }, -322 );
   }
   db.getRG( groupName ).stop();
   db.getRG( groupName ).start();
   commCheckBusinessStatus( db );
   db.invalidateCache();

   try
   {
      //设置preferedinstancemode为random
      var options = { PreferedInstance: instanceid, PreferedInstanceMode: "random" };
      checkAccessNodes( cl, expAccessNodes, options );

      //不设置preferedinstancemode
      options = { PreferedInstance: instanceid };
      checkAccessNodes( cl, expAccessNodes, options );

      //设置preferedinstancemode为ordered
      expAccessNodes.pop();
      options = { PreferedInstance: instanceid, PreferedInstanceMode: "ordered" };
      checkAccessNodes( cl, expAccessNodes, options );
      commDropCL( db, COMMCSNAME, clName, false, false );
   }
   finally
   {
      deleteConf( db, { instanceid: 1 }, { GroupName: groupName }, -264 );
      db.getRG( groupName ).stop();
      db.getRG( groupName ).start();
      commCheckBusinessStatus( db );
   }
}


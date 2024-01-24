/* *****************************************************************************
@description:  seqDB-14089: 设置会话访问属性，多值指定preferedinstance值instanceid部分存在，preferedinstanceMode覆盖不同值 
@author: 2018-1-24 wuyan  Init
***************************************************************************** */
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var clName = CHANGEDPREFIX + "_14089";
   var groups = commGetGroups( db );
   var group = groups[0];
   var groupName = group[0].GroupName;
   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName, { Group: groupName } );
   insertData( cl );

   var instanceid = [29, 255];
   var hostName = group[1].HostName;
   var svcName = group[1].svcname;
   updateConf( db, { instanceid: instanceid[1] }, { NodeName: hostName + ":" + svcName }, -322 );
   db.getRG( groupName ).getNode( hostName, svcName ).stop();
   db.getRG( groupName ).getNode( hostName, svcName ).start();

   try
   {
      commCheckBusinessStatus( db );
      db.invalidateCache();

      //设置preferedinstancemode为random
      var options = { PreferedInstance: instanceid, PreferedInstanceMode: "random" };
      var expAccessNodes = [hostName + ":" + svcName];
      checkAccessNodes( cl, expAccessNodes, options );

      //不设置preferedinstancemode
      options = { PreferedInstance: instanceid };
      checkAccessNodes( cl, expAccessNodes, options );

      //设置preferedinstancemode为ordered
      options = { PreferedInstance: instanceid, PreferedInstanceMode: "ordered" };
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


/* *****************************************************************************
@description: seqDB-18601: 设置会话访问属性，多值指定preferedinstance存在的同时设置preferedStrict 
@author：2019-6-20 luweikang  Init
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
   var clName = CHANGEDPREFIX + "_18601";

   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName, { Group: groupName } );
   insertData( cl );

   var instanceid = [8, 9, 10];
   for( var i = 0; i < instanceid.length; i++ )
   {
      var hostName = group[i + 1].HostName;
      var svcName = group[i + 1].svcname;
      updateConf( db, { instanceid: instanceid[i] }, { NodeName: hostName + ":" + svcName }, -322 );
   }
   db.getRG( groupName ).stop();
   db.getRG( groupName ).start();
   commCheckBusinessStatus( db );
   db.invalidateCache();

   db.getRG( groupName ).getNode( group[1].HostName, group[1].svcname ).stop();
   var expAccessNodes = [group[2].HostName + ":" + group[2].svcname];
   var options = { PreferedInstance: [instanceid[0], instanceid[1]], PreferedStrict: true };
   checkAccessNodes( cl, expAccessNodes, options );

   db.getRG( groupName ).getNode( group[2].HostName, group[2].svcname ).stop();
   db.setSessionAttr( options );
   try
   {
      cl.find().explain();
      throw new Error( "FIND_SHOULD_FAIL" );
   }
   catch( e )
   {
      if( e.message != SDB_CLS_NODE_BSFAULT )
      {
         throw e;
      }
   }

   expAccessNodes = [group[3].HostName + ":" + group[3].svcname];
   options = { PreferedInstance: [instanceid[0], instanceid[1]], PreferedStrict: false };
   checkAccessNodes( cl, expAccessNodes, options );

   db.getRG( groupName ).start();
   expAccessNodes = [group[1].HostName + ":" + group[1].svcname];
   options = { PreferedInstance: instanceid[0], PreferedStrict: true };
   checkAccessNodes( cl, expAccessNodes, options );

   deleteConf( db, { instanceid: 1 }, { GroupName: groupName }, -322 );
   db.getRG( groupName ).stop();
   db.getRG( groupName ).start();
   commCheckBusinessStatus( db );

   commDropCL( db, COMMCSNAME, clName, false, false );
}

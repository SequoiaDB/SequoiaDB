/* *****************************************************************************
@description: seqDB-18330:设置会话访问属性，单值指定preferedinstance存在的同时设置PreferedStrict 
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
   var clName = CHANGEDPREFIX + "_cl18330";

   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName, { Group: groupName } );
   insertData( cl );

   var instanceid = [9, 8, 10];
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

   var options = { PreferedInstance: instanceid[0], PreferedStrict: true };
   db.setSessionAttr( options );
   db.getRG( groupName ).getNode( group[1].HostName, group[1].svcname ).stop();
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

   options = { PreferedInstance: instanceid[0], PreferedStrict: false };
   db.setSessionAttr( options );
   cl.find().explain();

   db.getRG( groupName ).getNode( group[1].HostName, group[1].svcname ).start();
   commCheckBusinessStatus( db );

   options = { PreferedInstance: instanceid[0], PreferedStrict: true };
   var expAccessNodes = [group[1].HostName + ":" + group[1].svcname];
   checkAccessNodes( cl, expAccessNodes, options );

   deleteConf( db, { instanceid: 1 }, { GroupName: groupName }, -322 );
   db.getRG( groupName ).stop();
   db.getRG( groupName ).start();
   commCheckBusinessStatus( db );

   commDropCL( db, COMMCSNAME, clName, false, false );
}


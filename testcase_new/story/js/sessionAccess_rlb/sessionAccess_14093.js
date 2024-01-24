/* *****************************************************************************
@description:seqDB-14093:设置会话访问属性，指定preferedinstance为instanceid存在节点和[M/S/A/m/s/a/-M/-S/-A/-m/-s/-a]，preferedinstanceMode覆盖不同值 
@author: 2020-4-9 zhaoxiaoni  Init
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
   var clName = CHANGEDPREFIX + "_14093";
   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName, { Group: groupName } );
   insertData( cl );

   var instanceid = [30, 124, 8];
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

   groups = getGroupsWithNodeNum( 3 );
   group = groups[0];
   var primaryPos = group[0].PrimaryPos;
   var primaryNode = group[primaryPos].HostName + ":" + group[primaryPos].svcname;

   try
   {
      var expAccessNodes = [];
      expAccessNodes.push( primaryNode );
      var options = { PreferedInstance: [124, 8, 30, "M"], preferedinstanceMode: "random" };
      checkAccessNodes( cl, expAccessNodes, options );

      var options = { PreferedInstance: [124, 8, 30, "M"], preferedinstanceMode: "ordered" };
      checkAccessNodes( cl, expAccessNodes, options );

      options = { PreferedInstance: [124, 8, 30, "m"], preferedinstanceMode: "random" };
      checkAccessNodes( cl, expAccessNodes, options );

      options = { PreferedInstance: [124, 8, 30, "m"], preferedinstanceMode: "ordered" };
      checkAccessNodes( cl, expAccessNodes, options );

      expAccessNodes = [];
      for( var i = 1; i < group.length; i++ )
      {
         if( i !== primaryPos )
         {
            expAccessNodes.push( group[i]["HostName"] + ":" + group[i]["svcname"] );
         }
      }
      options = { PreferedInstance: [124, 8, 30, "S"], preferedinstanceMode: "random" };
      checkAccessNodes( cl, expAccessNodes, options );

      options = { PreferedInstance: [124, 8, 30, "s"], preferedinstanceMode: "random" };
      checkAccessNodes( cl, expAccessNodes, options );

      expAccessNodes.pop();
      options = { PreferedInstance: [30, 124, 8, "S"], preferedinstanceMode: "ordered" };//取的节点为备节点中靠前的数字
      checkAccessNodes( cl, expAccessNodes, options );

      options = { PreferedInstance: [30, 124, 8, "s"], preferedinstanceMode: "ordered" };
      checkAccessNodes( cl, expAccessNodes, options );

      expAccessNodes = getGroupNodes( groupName );
      options = { PreferedInstance: [124, 8, 30, "A"], preferedinstanceMode: "random" };
      checkAccessNodes( cl, expAccessNodes, options );

      options = { PreferedInstance: [124, 8, 30, "a"], preferedinstanceMode: "random" };
      checkAccessNodes( cl, expAccessNodes, options );

      options = { PreferedInstance: [124, 8, 30, "-M"], preferedinstanceMode: "random" };
      checkAccessNodes( cl, expAccessNodes, options );

      options = { PreferedInstance: [124, 8, 30, "-m"], preferedinstanceMode: "random" };
      checkAccessNodes( cl, expAccessNodes, options );

      options = { PreferedInstance: [124, 8, 30, "-S"], preferedinstanceMode: "random" };
      checkAccessNodes( cl, expAccessNodes, options );

      options = { PreferedInstance: [124, 8, 30, "-s"], preferedinstanceMode: "random" };
      checkAccessNodes( cl, expAccessNodes, options );

      options = { PreferedInstance: [124, 8, 30, "-A"], preferedinstanceMode: "random" };
      checkAccessNodes( cl, expAccessNodes, options );

      options = { PreferedInstance: [124, 8, 30, "-a"], preferedinstanceMode: "random" };
      checkAccessNodes( cl, expAccessNodes, options );

      expAccessNodes = [];
      expAccessNodes.push( group[2].HostName + ":" + group[2].svcname );

      options = { PreferedInstance: [124, 8, 30, "A"], preferedinstanceMode: "ordered" };
      checkAccessNodes( cl, expAccessNodes, options );

      options = { PreferedInstance: [124, 8, 30, "a"], preferedinstanceMode: "ordered" };
      checkAccessNodes( cl, expAccessNodes, options );

      options = { PreferedInstance: [124, 8, 30, "-M"], preferedinstanceMode: "ordered" };
      checkAccessNodes( cl, expAccessNodes, options );

      options = { PreferedInstance: [124, 8, 30, "-m"], preferedinstanceMode: "ordered" };
      checkAccessNodes( cl, expAccessNodes, options );

      options = { PreferedInstance: [124, 8, 30, "-S"], preferedinstanceMode: "ordered" };
      checkAccessNodes( cl, expAccessNodes, options );

      options = { PreferedInstance: [124, 8, 30, "-s"], preferedinstanceMode: "ordered" };
      checkAccessNodes( cl, expAccessNodes, options );

      options = { PreferedInstance: [124, 8, 30, "-A"], preferedinstanceMode: "ordered" };
      checkAccessNodes( cl, expAccessNodes, options );

      options = { PreferedInstance: [124, 8, 30, "-a"], preferedinstanceMode: "ordered" };
      checkAccessNodes( cl, expAccessNodes, options );
   }
   finally
   {
      deleteConf( db, { instanceid: 1 }, { GroupName: groupName }, -322 );
      db.getRG( groupName ).stop();
      db.getRG( groupName ).start();
      commCheckBusinessStatus( db );
   }

   commDropCL( db, COMMCSNAME, clName, false, false );
}



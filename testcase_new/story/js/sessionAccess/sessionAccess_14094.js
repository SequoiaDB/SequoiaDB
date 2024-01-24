/* *****************************************************************************
@description:  seqDB-14094:设置会话访问属性，指定preferedinstance值instanceid不存在对应节点和[S/M/A/s/m/a/-S/-M/-A/-s/-m/-a]
@author: 2020-4-9 zhaoxiaoni  Init
***************************************************************************** */
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;

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
   var primaryPos = group[0].PrimaryPos;
   var primaryNode = group[primaryPos].HostName + ":" + group[primaryPos].svcname;
   var clName = CHANGEDPREFIX + "_14094";

   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName, { Group: groupName, ReplSize: 0 } );
   insertData( cl );

   var expAccessNodes = [];
   expAccessNodes.push( primaryNode );
   var options = { PreferedInstance: [11, 224, 38, "M"] };
   checkAccessNodes( cl, expAccessNodes, options );

   options = { PreferedInstance: [11, 224, 38, "-M"] };
   checkAccessNodes( cl, expAccessNodes, options );

   options = { PreferedInstance: [11, 224, 38, "m"] };
   checkAccessNodes( cl, expAccessNodes, options );

   options = { PreferedInstance: [11, 224, 38, "-m"] };
   checkAccessNodes( cl, expAccessNodes, options );

   expAccessNodes = [];
   for( var i = 1; i < group.length; i++ )
   {
      if( i !== primaryPos )
      {
         expAccessNodes.push( group[i]["HostName"] + ":" + group[i]["svcname"] );
      }
   }
   options = { PreferedInstance: [11, 224, 38, "S"] };
   checkAccessNodes( cl, expAccessNodes, options );

   options = { PreferedInstance: [11, 224, 38, "-S"] };
   checkAccessNodes( cl, expAccessNodes, options );

   options = { PreferedInstance: [11, 224, 38, "s"] };
   checkAccessNodes( cl, expAccessNodes, options );

   options = { PreferedInstance: [11, 224, 38, "-s"] };
   checkAccessNodes( cl, expAccessNodes, options );

   expAccessNodes = getGroupNodes( groupName );
   options = { PreferedInstance: [11, 224, 38, "A"] };
   checkAccessNodes( cl, expAccessNodes, options );

   options = { PreferedInstance: [11, 224, 38, "-A"] };
   checkAccessNodes( cl, expAccessNodes, options );

   options = { PreferedInstance: [11, 224, 38, "a"] };
   checkAccessNodes( cl, expAccessNodes, options );

   options = { PreferedInstance: [11, 224, 38, "-a"] };
   checkAccessNodes( cl, expAccessNodes, options );

   commDropCL( db, COMMCSNAME, clName, false, false );
}


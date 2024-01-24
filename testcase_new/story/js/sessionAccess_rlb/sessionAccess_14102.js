/* *****************************************************************************
@description: seqDB-14102:设置会话访问属性，指定preferedinstance值instanceid所在节点为主/备，同时指定S/M 
@author: 2020-5-11 Zhao Xiaoni  Init
***************************************************************************** */
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var nodeNum = 3;
   var groupName = "rg_14102";
   var clName = CHANGEDPREFIX + "_14102";
   var hostName = commGetGroups( db )[0][1].HostName;
   var nodeInfos = commCreateRG( db, groupName, nodeNum, hostName );
   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName, { Group: groupName, ReplSize: 0 } );
   insertData( cl );

   var expAccessNodes = [nodeInfos[2].hostname + ":" + nodeInfos[2].svcname];
   var options = { PreferedInstance: [3, "S"] };
   checkAccessNodes( cl, expAccessNodes, options );

   expAccessNodes = [nodeInfos[0].hostname + ":" + nodeInfos[0].svcname];
   options = { PreferedInstance: [1, "M"] };
   checkAccessNodes( cl, expAccessNodes, options );

   commDropCL( db, COMMCSNAME, clName, false, false );
   db.removeRG( groupName );
}


/* *****************************************************************************
@description: seqDB-14083:设置会话属性，选取实例instanceid值不存在 
@author: 2018-1-22 wuyan  Init
***************************************************************************** */
testConf.skipStandAlone = true;
testConf.clOpt = { ReplSize: 0 };
testConf.clName = CHANGEDPREFIX + "_14083";
testConf.useSrcGroup = true;

main( test );

function test ( testPara )
{
   var groupName = testPara.srcGroupName;
   var group = commGetGroups( db, "", groupName )[0];
   insertData( testPara.testCL );

   var instanceid = 40;
   var options = { PreferedInstance: instanceid };
   var nodes = group.slice( 1 ).sort( sortBy( "NodeID" ) );
   var index = ( instanceid - 1 ) % ( nodes.length );
   var expAccessNodes = [nodes[index].HostName + ":" + nodes[index].svcname];
   checkAccessNodes( testPara.testCL, expAccessNodes, options );

   options = { PreferedInstance: [39, 12] };
   expAccessNodes = getGroupNodes( groupName );
   checkAccessNodes( testPara.testCL, expAccessNodes, options );
}

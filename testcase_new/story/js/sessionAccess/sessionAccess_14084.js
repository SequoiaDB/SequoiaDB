/* *****************************************************************************
@description: seqDB-14084:设置会话属性，选取实例为节点下标值
@author: 2018-1-24 wuyan  Init
***************************************************************************** */
testConf.skipStandAlone = true;
testConf.skipOneDuplicatePerGroup = true;
testConf.clName = CHANGEDPREFIX + "_14084";
testConf.clOpt = { ReplSize: 0 };
testConf.useSrcGroup = true;
main( test );

function test ( testPara )
{
   var groupName = testPara.srcGroupName;
   var group = commGetGroups( db, "", groupName )[0].sort( sortBy( "NodeID" ) );
   insertData( testPara.testCL );

   //节点没有配置instanceid的情况下，按照节点的nodeid在组内的排序序列（从1开始）作为instanceid来进行选取
   var instanceid = Math.floor( Math.random() * ( group.length - 1 ) + 1 );
   var options = { PreferedInstance: instanceid };
   var expAccessNodes = [group[instanceid].HostName + ":" + group[instanceid].svcname];
   checkAccessNodes( testPara.testCL, expAccessNodes, options );
}


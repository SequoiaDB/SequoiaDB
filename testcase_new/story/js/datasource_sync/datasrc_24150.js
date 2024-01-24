/******************************************************************************
 * @Description   : seqDB-24150:源集群设置会话访问属性，指定preferedinstance为节点下标值
 * @Author        : Wu Yan
 * @CreateTime    : 2021.05.06
 * @LastEditTime  : 2021.06.07
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneDuplicatePerGroup = true;

main( test );

function test ()
{
   var dataSrcName = "datasrc24150";
   var clNameA = "datasource24150A";
   var clNameB = "datasource24150B";
   var srcCSName = "datasrcCS_24150";
   var csNameA = "DS_24150A";
   var csNameB = "DS_24150B";
   commDropCS( datasrcDB, srcCSName );
   clearDataSource( [ csNameA, csNameB ], dataSrcName );
   commCreateCS( datasrcDB, srcCSName );
   var group = commGetGroups( datasrcDB )[0].sort( sortBy( "NodeID" ) );
   var groupName = group[0].GroupName;
   commCreateCL( datasrcDB, srcCSName, clNameA, { ShardingKey: { a: 1 }, ReplSize: -1, Group: groupName } );
   commCreateCL( datasrcDB, srcCSName, clNameB, { ShardingKey: { a: 1 }, ReplSize: -1, Group: groupName } );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );

   //集合空间级映射
   var csA = db.createCS( csNameA, { DataSource: dataSrcName, Mapping: srcCSName });
   var dbclA = csA.getCL(clNameA);
   //集合级映射
   var csB = db.createCS( csNameB );    
   var dbclB = csB.createCL( clNameB, { DataSource: dataSrcName, Mapping: srcCSName + "." + clNameB } );   
   var docs = [{ a: 1, b: 1, c: "testc" }, { a: 2, b: [1, "test"], c: "testsgasdgasdg" }, { a: 3, b: 234.3, c: { a: 1 } }, { a: 4, b: 4, c: "testtess4" }];
   dbclA.insert( docs );
   dbclB.insert( docs );
   commCheckLSN( datasrcDB, groupName );

   //节点没有配置instanceid的情况下，按照节点的nodeid在组内的排序序列（从1开始）作为instanceid来进行选取
   var instanceid = Math.floor( Math.random() * ( group.length - 1 ) + 1 );
   var options = { PreferedInstance: instanceid };
   var expAccessNodes = [group[instanceid].HostName + ":" + group[instanceid].svcname];
   setSessionAndcheckAccessNodes( dbclA, expAccessNodes, options );
   setSessionAndcheckAccessNodes( dbclB, expAccessNodes, options );

   datasrcDB.dropCS( srcCSName );
   datasrcDB.close();
   clearDataSource( [ csNameA, csNameB ], dataSrcName );
}


/******************************************************************************
 * @Description   : seqDB-24361:主表创建/删除本地索引，其中子表为切分表 
 * @Author        : liuli
 * @CreateTime    : 2021.09.26
 * @LastEditTime  : 2021.10.26
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;

main( test );
function test ()
{
   var csName = "cs_24361";
   var mainCLName = "maincl_24361";
   var subCLName1 = "subcl_24361_1";
   var subCLName2 = "subcl_24361_2";
   var indexName1 = "index_24361_1";
   var indexName2 = "index_24361_2";
   var groupNames = commGetDataGroupNames( db );

   commDropCS( db, csName );
   var maincl = commCreateCL( db, csName, mainCLName, { IsMainCL: true, ShardingKey: { a: 1 }, ShardingType: "range", ReplSize: 0 } );
   var subcl1 = commCreateCL( db, csName, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash", Group: groupNames[0] } );
   commCreateCL( db, csName, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   maincl.attachCL( csName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 5000 } } );
   maincl.attachCL( csName + "." + subCLName2, { LowBound: { a: 5000 }, UpBound: { a: 10000 } } );

   // 主表插入数据，子表进行切分
   insertBulkData( maincl, 10000 );
   subcl1.split( groupNames[0], groupNames[1], 50 );

   // 获取原组上一个节点，指定节点创建独立索引
   var nodeName = getOneNodeName( db, groupNames[0] );
   maincl.createIndex( indexName1, { c: 1 }, { Standalone: true }, { NodeName: nodeName } );

   // 校验索引并查看访问计划
   checkStandaloneIndexOnNode( db, csName, subCLName1, indexName1, nodeName, true );
   checkExplainUseStandAloneIndex( csName, subCLName1, nodeName, { c: 1 }, "ixscan", indexName1 );

   // 指定原组和目标组各一个节点创建独立索引
   var nodeName2 = getOneNodeName( db, groupNames[1] );
   maincl.createIndex( indexName2, { no: 1 }, { Standalone: true }, { NodeName: [nodeName, nodeName2] } );

   // 校验索引并查看访问计划
   checkStandaloneIndexOnNode( db, csName, subCLName1, indexName2, [nodeName, nodeName2], true );
   checkExplainUseStandAloneIndex( csName, subCLName1, nodeName, { no: 1 }, "ixscan", indexName2 );
   checkExplainUseStandAloneIndex( csName, subCLName1, nodeName2, { no: 1 }, "ixscan", indexName2 );

   //清除环境
   commDropCS( db, csName );
}
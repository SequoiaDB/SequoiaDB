/******************************************************************************
 * @Description   : seqDB-24356 :: 普通表创建/删除本地索引(指定nodeName)    
 * @Author        : wu yan
 * @CreateTime    : 2021.09.23
 * @LastEditTime  : 2022.01.29
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.clOpt = { ReplSize: -1 };
testConf.clName = COMMCLNAME + "_standaloneIndex_24356";

main( test );
function test ( testPara )
{
   var indexName = "Index_24356";
   var recordNum = 1000;
   var dbcl = testPara.testCL;
   var fullclName = COMMCSNAME + "." + testConf.clName;

   var nodeName = getCLOneNodeName( db, fullclName );
   dbcl.createIndex( indexName, { a: 1, b: 1 }, { Standalone: true }, { NodeName: nodeName } );
   insertBulkData( dbcl, recordNum );

   //检查任务信息    
   checkStandaloneIndexTask( "Create index", fullclName, nodeName, indexName );

   //检查索引信息
   checkStandaloneIndexOnNode( db, COMMCSNAME, testConf.clName, indexName, nodeName, true );
   checkExplainUseStandAloneIndex( COMMCSNAME, testConf.clName, nodeName, { a: 10, b: 10 }, "ixscan", indexName );

   //删除本地索引
   dbcl.dropIndex( indexName );
   checkStandaloneIndexTask( "Drop index", fullclName, nodeName, indexName );
   checkStandaloneIndexOnNode( db, COMMCSNAME, testConf.clName, indexName, nodeName, false );
   checkExplainUseStandAloneIndex( COMMCSNAME, testConf.clName, nodeName, { a: 10, b: 10 }, "tbscan", "" );
}


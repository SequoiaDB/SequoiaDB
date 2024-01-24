/******************************************************************************
 * @Description   : seqDB-24366:主节点存在本地索引，创建/删除相同一致性索引    
 * @Author        : wu yan
 * @CreateTime    : 2021.09.26
 * @LastEditTime  : 2022.01.29
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.skipOneDuplicatePerGroup = true
testConf.clName = COMMCLNAME + "_standaloneIndex_24366";
testConf.clOpt = { ShardingKey: { no: 1 }, ShardingType: 'range', ReplSize: 0 };

main( test );
function test ()
{
   var indexName = "Index_24366a";
   var recordNum = 1000;
   var dbcl = testPara.testCL;
   var groupName = testPara.srcGroupName;
   insertBulkData( dbcl, recordNum );

   var nodeInfo = db.getRG( groupName ).getMaster();
   var nodeName = nodeInfo["_nodename"];
   dbcl.createIndex( indexName, { no: 1, b: 1 }, { Standalone: true }, { NodeName: nodeName } );

   //场景b：创建一致性索引，其中索引名相同，索引定义不同   
   assert.tryThrow( SDB_IXM_EXIST, function()
   {
      dbcl.createIndex( indexName, { no: -1 } );
   } );
   checkIndexTaskResult( "Create index", COMMCSNAME, testConf.clName, indexName, SDB_IXM_EXIST );
   checkStandaloneIndexOnNode( db, COMMCSNAME, testConf.clName, indexName, nodeName, true );

   //场景c：创建一致性索引，其中索引名不同，索引定义相同  
   var indexNameC = "testindexc_24366";
   assert.tryThrow( SDB_IXM_EXIST_COVERD_ONE, function()
   {
      dbcl.createIndex( indexNameC, { no: 1, b: 1 } );
   } );
   checkIndexTaskResult( "Create index", COMMCSNAME, testConf.clName, indexNameC, SDB_IXM_EXIST_COVERD_ONE );
   checkStandaloneIndexOnNode( db, COMMCSNAME, testConf.clName, indexName, nodeName, true );
   checkIndexExist( db, COMMCSNAME, testConf.clName, indexNameC, false );

   //场景a：创建一致性索引，其中索引名和索引定义都相同
   dbcl.createIndex( indexName, { no: 1, b: 1 } );
   checkIndexTaskResult( "Create index", COMMCSNAME, testConf.clName, indexName, 0 );
   checkExistIndexConsistent( db, COMMCSNAME, testConf.clName, indexName );

   //删除本地索引
   dbcl.dropIndex( indexName );
   checkIndexTask( "Drop index", COMMCSNAME, testConf.clName, indexName );
   commCheckIndexConsistent( db, COMMCSNAME, testConf.clName, indexName, false );
}


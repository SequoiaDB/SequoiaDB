/******************************************************************************
 * @Description   : seqDB-24368:相同节点创建相同本地索引 
 * @Author        : liuli
 * @CreateTime    : 2021.09.27
 * @LastEditTime  : 2022.01.13
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_24368";
testConf.clOpt = { ReplSize: 0 };

main( test );
function test ( args )
{
   var dbcl = args.testCL;
   var indexName1 = "index_24368_1";
   var indexName2 = "index_24368_2";
   var indexName3 = "index_24368_3";

   // 插入数据后创建本地索引
   insertBulkData( dbcl, 1000 );
   var nodeName = getCLOneNodeName( db, COMMCSNAME + "." + testConf.clName );
   dbcl.createIndex( indexName1, { a: 1 }, { Standalone: true }, { NodeName: nodeName } );

   // 再次创建索引，索引名相同，索引定义不同
   assert.tryThrow( SDB_IXM_EXIST, function()
   {
      dbcl.createIndex( indexName1, { c: 1 }, { Standalone: true }, { NodeName: nodeName } );
   } );

   // 再次创建索引，索引名不同，索引定义相同
   assert.tryThrow( SDB_IXM_EXIST_COVERD_ONE, function()
   {
      dbcl.createIndex( indexName2, { a: 1 }, { Standalone: true }, { NodeName: nodeName } );
   } );

   // 再次创建索引，索引名和索引定义都相同
   assert.tryThrow( SDB_IXM_REDEF, function()
   {
      dbcl.createIndex( indexName1, { a: 1 }, { Standalone: true }, { NodeName: nodeName } );
   } );

   // 再次创建索引，索引名不同，索引定义部分相同，包含重复字段
   dbcl.createIndex( indexName2, { a: 1, c: 1 }, { Standalone: true }, { NodeName: nodeName } );

   // 索引名不同，索引定义部分相同，字段相同排序不同
   dbcl.createIndex( indexName3, { a: -1 }, { Standalone: true }, { NodeName: nodeName } );

   // 校验索引
   checkStandaloneIndexOnNode( db, COMMCSNAME, testConf.clName, indexName1, nodeName, true );
   checkIndexKey( dbcl, indexName1, { a: 1 } );
   checkStandaloneIndexOnNode( db, COMMCSNAME, testConf.clName, indexName2, nodeName, true );
   checkIndexKey( dbcl, indexName2, { a: 1, c: 1 } );
   checkStandaloneIndexOnNode( db, COMMCSNAME, testConf.clName, indexName3, nodeName, true );
   checkIndexKey( dbcl, indexName3, { a: -1 } );

   // 查询访问计划
   checkExplainUseStandAloneIndex( COMMCSNAME, testConf.clName, nodeName, { a: 5 }, "ixscan", indexName1 );
   checkExplainUseStandAloneIndex( COMMCSNAME, testConf.clName, nodeName, { a: 5, c: 5 }, "ixscan", indexName2 );
}

function checkIndexKey ( dbcl, indexName, indexDef )
{
   var cursor = dbcl.snapshotIndexes( { "IndexDef.name": indexName } );
   var key = cursor.next().toObj().IndexDef.key;
   cursor.close();
   assert.equal( key, indexDef );
}
/******************************************************************************
 * @Description   : seqDB-24358 :: 创建本地非普通索引   
 * @Author        : wu yan
 * @CreateTime    : 2021.09.23
 * @LastEditTime  : 2021.12.17
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_standaloneIndex_24358";
testConf.clOpt = { ShardingKey: { no: 1 }, ShardingType: 'range', ReplSize: 0 };

main( test );
function test ( testPara )
{
   var indexName = "Index_24358";
   var recordNum = 1000;
   var dbcl = testPara.testCL;
   var fullclName = COMMCSNAME + "." + testConf.clName;
   insertBulkData( dbcl, recordNum );

   var nodeName = getCLOneNodeName( db, fullclName );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.createIndex( indexName, { a: 1, b: 1 }, { Unique: true, Standalone: true }, { NodeName: nodeName } );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.createIndex( indexName, { a: 1, b: 1 }, { Unique: true, Enforced: true, Standalone: true }, { NodeName: nodeName } );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.createIndex( indexName, { a: 1, b: 1 }, { Enforced: true, Standalone: true }, { NodeName: nodeName } );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.createIndex( indexName, { a: 1, b: 1 }, { NotNull: true, Standalone: true }, { NodeName: nodeName } );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.createIndex( indexName, { a: "text", b: 1 }, { Standalone: true }, { NodeName: nodeName } );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.createIndex( indexName, { a: 1, b: 1 }, { Global: true, Standalone: true }, { NodeName: nodeName } );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.createIndex( indexName, { a: 1, b: 1 }, { NotArray: true, Standalone: true }, { NodeName: nodeName } );
   } );

   //检查索引信息
   checkStandaloneIndexOnNode( db, COMMCSNAME, testConf.clName, indexName, nodeName, false );
}


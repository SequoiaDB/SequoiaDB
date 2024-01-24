/******************************************************************************
 * @Description   : seqDB-23752:重复创建相同索引
 * @Author        : Yi Pan
 * @CreateTime    : 2021.03.29
 * @LastEditTime  : 2021.10.26
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_23752";

main( test );
function test ( testPara )
{
   var indexName1 = "index_23752_1";
   var indexName2 = "index_23752_2";
   var indexName3 = "index_23752_3";

   var cl = testPara.testCL;

   // 创建索引
   cl.createIndex( indexName1, { "a": 1 } );

   // 插入数据
   var expResult = insertBulkData( cl, 300 );

   // 再次创建索引 a.索引名相同，索引定义不相同
   assert.tryThrow( SDB_IXM_EXIST, function()
   {
      cl.createIndex( indexName1, { "b": 1 } );
   } );
   checkNoTask( "Create index", COMMCSNAME, testConf.clName, indexName1, SDB_IXM_EXIST );

   // 再次创建索引 b.索引名不同，索引定义相同
   assert.tryThrow( SDB_IXM_EXIST_COVERD_ONE, function()
   {
      cl.createIndex( indexName2, { "a": 1 } );
   } );
   checkNoTask( "Create index", COMMCSNAME, testConf.clName, indexName2, SDB_IXM_EXIST_COVERD_ONE );

   // 再次创建索引 c.索引名和索引定义都相同
   assert.tryThrow( SDB_IXM_REDEF, function()
   {
      cl.createIndex( indexName1, { "a": 1 } );
   } );
   checkNoTask( "Create index", COMMCSNAME, testConf.clName, indexName1, SDB_IXM_REDEF );

   // 再次创建索引 d.索引名不同，索引定义部分相同
   cl.createIndex( indexName2, { "a": 1, "c": 1 } );
   cl.createIndex( indexName3, { "a": -1 } );
   checkIndexTaskResult( "Create index", COMMCSNAME, testConf.clName, [indexName1, indexName2, indexName3], 0 );

   // 校验索引一致性
   commCheckIndexConsistent( db, COMMCSNAME, testConf.clName, indexName1, true );
   commCheckIndexConsistent( db, COMMCSNAME, testConf.clName, indexName2, true );
   commCheckIndexConsistent( db, COMMCSNAME, testConf.clName, indexName3, true );

   // 查询数据并校验
   var actResult = cl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   checkExplain( cl, { a: 5, c: 5 }, "ixscan", indexName2 );
}

function checkNoTask ( taskTypeDesc, csName, clName, indexName, resultCode )
{
   var cursor = db.listTasks( { Name: csName + "." + clName, TaskTypeDesc: taskTypeDesc, IndexName: indexName, ResultCode: resultCode } );
   while( cursor.next() )
   {
      var taskInfo = cursor.current().toObj();
      throw new Error( "check task should be no exist! act task= " + JSON.stringify( taskInfo ) );
   }
   cursor.close();
}
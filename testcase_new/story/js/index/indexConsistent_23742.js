/******************************************************************************
 * @Description   : seqDB-23742:普通表异步创建唯一索引
 * @Author        : Yi Pan
 * @CreateTime    : 2021.03.29
 * @LastEditTime  : 2021.12.10
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_23742";

main( test );
function test ( testPara )
{
   var cl = testPara.testCL;
   var indexName = "idx23742";

   //1.插入数据
   var expResult = insertBulkData( cl, 100 );

   //2.异步方式创建唯一索引
   var taskid = cl.createIndexAsync( indexName, { "a": 1 }, { "Unique": true, "Enforced": true } );

   //3.等待任务结束并校验
   db.waitTasks( taskid );
   checkIndexTask( "Create index", COMMCSNAME, testConf.clName, indexName, 0 );
   commCheckIndexConsistent( db, COMMCSNAME, testConf.clName, indexName, true );

   //4.插入索引字段重复数据
   assert.tryThrow( SDB_IXM_DUP_KEY, function()
   {
      cl.insert( { "a": 1 } );
   } );

   //5.查询数据，查看访问计划
   var actResult = cl.find().sort( { a: 1 } ).hint( { "": indexName } );
   commCompareResults( actResult, expResult );
   checkExplain( cl, { "a": 5 }, "ixscan", indexName );
}
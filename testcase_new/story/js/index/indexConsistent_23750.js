/******************************************************************************
 * @Description   : seqDB-23750:事务中创建/删除索引
 * @Author        : Yi Pan
 * @CreateTime    : 2021.03.29
 * @LastEditTime  : 2021.09.22
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_23750";

main( test );
function test ( testPara )
{
   var indexName1 = "idx23750_1";
   var indexName2 = "idx23750_2";
   var cl = testPara.testCL;

   //1.开启事务
   db.transBegin();

   //2.插入数据
   var docs = insertBulkData( cl, 5000 );
   cl.insert( { "a": 5000, "c": 100 } );
   docs.push( { "a": 5000, "c": 100 } );

   //3.创建普通索引和唯一索引
   cl.createIndex( indexName1, { "c": 1 } );
   cl.createIndex( indexName2, { "a": 1 }, { "Unique": true, "Enforced": true } );

   //4.提交事务
   db.transCommit();

   //5.校验任务信息和索引一致性
   checkIndexTask( "Create index", COMMCSNAME, testConf.clName, [indexName1, indexName2], 0 );
   commCheckIndexConsistent( db, COMMCSNAME, testConf.clName, indexName1, true );
   commCheckIndexConsistent( db, COMMCSNAME, testConf.clName, indexName2, true );

   docs.sort( sortBy( "a" ) );
   //6.索引1查询数据，查看访问计划
   var actResult = cl.find().sort( { "a": 1 } );
   commCompareResults( actResult, docs );
   checkExplain( cl, { "c": 5 }, "ixscan", indexName1 );

   //6.索引2查询数据，查看访问计划
   var actResult = cl.find().sort( { "a": 1 } );
   commCompareResults( actResult, docs );
   checkExplain( cl, { "a": 5 }, "ixscan", indexName2 );

   //7.开启事务
   db.transBegin();

   //8.删除索引
   cl.dropIndex( indexName1 );
   cl.dropIndex( indexName2 );

   //9.提交事务
   db.transCommit();

   //10.校验任务和主备节点索引被删除
   checkIndexTask( "Drop index", COMMCSNAME, testConf.clName, [indexName1, indexName2], 0 );
   commCheckIndexConsistent( db, COMMCSNAME, testConf.clName, indexName1, false );
   commCheckIndexConsistent( db, COMMCSNAME, testConf.clName, indexName2, false );
}
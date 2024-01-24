/******************************************************************************
 * @Description   : seqDB-23741:普通表创建/删除普通索引
 * @Author        : Yi Pan
 * @CreateTime    : 2021.03.29
 * @LastEditTime  : 2021.09.18
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_23741";

main( test );

function test ( testPara )
{
   var indexName = "idx23741";
   var cl = testPara.testCL;

   //1.创建普通索引
   cl.createIndex( indexName, { "a": 1 } );

   //2.查看索引任务信息
   checkIndexTask( "Create index", COMMCSNAME, testConf.clName, indexName, 0 );
   commCheckIndexConsistent( db, COMMCSNAME, testConf.clName, indexName, true );

   //3.插入数据
   var expResult = insertBulkData( cl, 100 );

   //4.查询数据，查看访问计划
   var actResult = cl.find().sort( { a: 1 } ).hint( { "": indexName } );
   commCompareResults( actResult, expResult );
   checkExplain( cl, { "a": 5 }, "ixscan", indexName );

   //5.删除索引
   cl.dropIndex( indexName );

   //6.查看索引任务信息
   checkIndexTask( "Drop index", COMMCSNAME, testConf.clName, indexName, 0 );
   commCheckIndexConsistent( db, COMMCSNAME, testConf.clName, indexName, false );

   //7.查询数据，查看访问计划
   var actResult = cl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   checkExplain( cl, { "a": 5 }, "tbscan" );
}
/******************************************************************************
 * @Description   : seqDB-24308:事务中创建/删除索引，回滚事务
 * @Author        : liuli
 * @CreateTime    : 2021.08.02
 * @LastEditTime  : 2021.09.23
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_24308";

main( test );
function test ( args )
{
   var indexName1 = "Index_24308_1";
   var indexName2 = "Index_24308_2";
   var dbcl = args.testCL;

   var docs = insertBulkData( dbcl, 10000 );

   // 开启事务后插入数据
   db.transBegin();
   var docs2 = [];
   for( var i = 10000; i < 20000; i++ )
   {
      var cValue = parseInt( Math.random() * 5000 );
      docs2.push( { a: i, b: i, c: cValue } );
   }
   dbcl.insert( docs2 );

   dbcl.createIndex( indexName1, { c: 1 } );
   dbcl.createIndex( indexName2, { a: 1 }, true );
   db.transRollback();

   // 校验任务和索引一致性
   checkIndexTask( "Create index", COMMCSNAME, testConf.clName, [indexName1, indexName2], 0 );
   commCheckIndexConsistent( db, COMMCSNAME, testConf.clName, indexName1, true );
   commCheckIndexConsistent( db, COMMCSNAME, testConf.clName, indexName2, true );

   var cursor = testPara.testCL.find( {}, { "_id": { "$include": 0 } } ).sort( { "a": 1 } ).hint( { "": indexName1 } );
   docs.sort( sortBy( "a" ) );
   commCompareResults( cursor, docs );

   var cursor = testPara.testCL.find( {}, { "_id": { "$include": 0 } } ).sort( { "c": 1 } ).hint( { "": indexName2 } );
   docs.sort( sortBy( "c" ) );
   commCompareResults( cursor, docs );

   checkExplain( dbcl, { a: 5 }, "ixscan", indexName2 );
   checkExplain( dbcl, { c: 5 }, "ixscan", indexName1 );

   db.transBegin();
   dbcl.dropIndex( indexName1 );
   dbcl.dropIndex( indexName2 );
   db.transRollback();

   checkIndexTask( "Drop index", COMMCSNAME, testConf.clName, [indexName1, indexName2], 0 );
   commCheckIndexConsistent( db, COMMCSNAME, testConf.clName, indexName1, false );
   commCheckIndexConsistent( db, COMMCSNAME, testConf.clName, indexName2, false );
}
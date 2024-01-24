/******************************************************************************
 * @Description   : seqDB-32581:复合索引，查询条件 $in 取值多，走索引扫描
 * @Author        : tangtao
 * @CreateTime    : 2023.07.26
 * @LastEditTime  : 2023.07.26
 * @LastEditors   : tangtao
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_32581";

main( test );
function test ( testPara )
{
   var cl = testPara.testCL;
   var idxName = "index_32581";

   var batchSize = 10000;
   for( j = 0; j < 5; j++ )
   {
      data = [];
      for( i = 0; i < batchSize; i++ )
         data.push( {
            a: i + j * batchSize,
            b: i + j * batchSize,
            c: i + j * batchSize,
            d: i + j * batchSize,
            e: i + j * batchSize,
         } );
      cl.insert( data );
   }

   cl.createIndex( idxName, { "a": 1, "b": 1, "c": 1 } );

   data = [];
   for( i = 0; i < 1000; i++ )
   {
      data.push( i * 10 );
   }

   var query1 = { a: { $in: data }, b: 1, c: 1 };
   var query2 = { a: { $in: data } };
   var indexName = "";
   indexName = cl.find( query1 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, idxName );
   indexName = cl.find( query2 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, idxName );

   db.analyze( { Collection: testConf.csName + "." + testConf.clName } );

   indexName = cl.find( query1 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, idxName );
   indexName = cl.find( query2 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, idxName );
}
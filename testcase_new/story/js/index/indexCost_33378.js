/******************************************************************************
 * @Description   : seqDB-33378:创建复合索引，查询条件为复合索引字段时索引选择
 * @Author        : wuyan
 * @CreateTime    : 2022.09.12
 * @LastEditTime  : 2023.09.13
 * @LastEditors   : wu yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_index33378";

main( test );
function test ()
{
   var cl = testPara.testCL;
   insertData( cl );

   var indexabc = "index_abc";
   var indexbc = "index_bc";
   cl.createIndex( indexabc, { a: 1, b: 1, c: 1 } );
   cl.createIndex( indexbc, { b: 1, c: 1 } );

   //查询条件包含复合索引部分字段和非索引字段，其中a字段使用$in匹配多个值，选择复合索引indexabc
   var queryConf = { "a": { $in: [0, 100] }, b: 0, d: 0 };
   var indexName = cl.find( queryConf ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, indexabc, "use index is " + indexName );

   //执行analyze收集统计信息后，再次查询  
   db.analyze( { Collection: COMMCSNAME + "." + testConf.clName } );
   var indexName = cl.find( queryConf ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, indexabc, "use index is " + indexName );
}

function insertData ( dbcl )
{
   var batchSize = 10000;
   for( j = 0; j < 5; j++ ) 
   {
      data = [];
      for( i = 0; i < batchSize; i++ )
      {
         data.push( {
            a: i + j * batchSize,
            b: i + j * batchSize,
            c: i + j * batchSize,
            d: i + j * batchSize,
            e: i + j * batchSize,
         } );
      }
      dbcl.insert( data );
   }
}


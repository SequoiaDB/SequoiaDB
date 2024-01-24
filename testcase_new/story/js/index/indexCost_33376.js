/******************************************************************************
 * @Description   : seqDB-33376:创建复合索引和单键索引，查询条件为复合索引字段且使用$in包含多个值时索引选择
 * @Author        : wuyan
 * @CreateTime    : 2022.09.12
 * @LastEditTime  : 2023.09.13
 * @LastEditors   : wu yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_index33376";

main( test );
function test ()
{
   var cl = testPara.testCL;
   insertData( cl );

   var indexabcd = "index_abcd";
   var indexab = "index_ab";
   var indexb = "index_b";
   cl.createIndex( indexabcd, { a: 1, c: 1, b: 1, d: 1 } );
   cl.createIndex( indexab, { a: 1, b: 1 } );
   cl.createIndex( indexb, { b: 1 } );

   var aValues = [];
   for( var i = 0; i < 100; i++ )
   {
      aValues.push( i * 10 );
   }
   var queryConf1 = { "a": { $in: aValues } };
   var queryConf2 = { "a": { $in: aValues }, b: 1 };

   //查询条件匹配复合索引字段且使用$in包含多个值，且包含所有索引相同字段时，选择复合索引indexab  
   var indexName = cl.find( queryConf1 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, indexab, "use index is " + indexName );

   var indexName = cl.find( queryConf2 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, indexab, "use index is " + indexName );

   //执行analyze收集统计信息后，再次查询  
   db.analyze( { Collection: COMMCSNAME + "." + testConf.clName } );
   var indexName = cl.find( queryConf1 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, indexab, "use index is " + indexName );

   var indexName = cl.find( queryConf2 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, indexab, "use index is " + indexName );
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


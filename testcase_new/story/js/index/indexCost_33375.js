/******************************************************************************
 * @Description   : seqDB-33375:创建唯一索引和普通索引，查询条件为唯一索引字段且使用$in包含多个值时索引选择
 * @Author        : wuyan
 * @CreateTime    : 2022.09.12
 * @LastEditTime  : 2023.09.12
 * @LastEditors   : wu yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_index33375";

main( test );
function test ()
{
   var cl = testPara.testCL;
   insertData( cl );

   var indexa = "indexa";
   var indexb = "indexb";
   cl.createIndex( indexa, { "a": 1 }, { unique: true } );
   cl.createIndex( indexb, { "b": 1 } );


   var aValues = [];
   for( var i = 0; i < 100; i++ )
   {
      aValues.push( i * 10 );
   }
   var queryConf1 = { "a": { $in: aValues } };
   var queryConf2 = { "a": { $in: aValues }, b: 1, c: 1 };
   var queryConf3 = { "a": { $in: aValues }, b: 0, c: 1 };

   //查询条件匹配唯一索引，且使用$in包含多个值时，选择唯一索引   
   var indexName = cl.find( queryConf1 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, indexa, "use index is " + indexName );

   var indexName = cl.find( queryConf2 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, indexa, "use index is " + indexName );

   var indexName = cl.find( queryConf3 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, indexa, "use index is " + indexName );

   //执行analyze收集统计信息后，再次查询  
   db.analyze( { Collection: COMMCSNAME + "." + testConf.clName } );
   var indexName = cl.find( queryConf1 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, indexa, "use index is " + indexName );

   var indexName = cl.find( queryConf2 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, indexb, "use index is " + indexName );

   var indexName = cl.find( queryConf3 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, indexa, "use index is " + indexName );
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
            b: 0,
            c: i + j * batchSize,
            d: i + j * batchSize,
            e: i + j * batchSize,
         } );
      }
      dbcl.insert( data );
   }
}


/******************************************************************************
 * @Description   : seqDB-32582:多个复合索引，查询条件覆盖最多的字段，正确选择索引
 * @Author        : tangtao
 * @CreateTime    : 2023.07.26
 * @LastEditTime  : 2023.07.26
 * @LastEditors   : tangtao
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.clName = COMMCLNAME + "_32582";

main( test );
function test ( testPara )
{
   var cl = testPara.testCL;
   var idxName1 = "index_32582_abcd";
   var idxName2 = "index_32582_abcde";

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

   cl.createIndex( idxName1, { "a": 1, "b": 1, "c": 1, "d": 1 } );
   cl.createIndex( idxName2, { "a": 1, "b": 1, "c": 1, "d": 1, "e": 1 } );

   var srcGroupName = testPara.srcGroupName;
   var dbNode = db.getRG( srcGroupName ).getMaster().connect();
   var dbNodeCL = dbNode.getCS( testConf.csName ).getCL( testConf.clName );
   var dbNodeCLStat = dbNode.getCS( "SYSSTAT" ).getCL( "SYSCOLLECTIONSTAT" );
   var dbNodeIXStat = dbNode.getCS( "SYSSTAT" ).getCL( "SYSINDEXSTAT" );

   var query1 = { a: 1, b: 1, c: 1, d: 1, e: 1 };
   var query2 = { a: 1, b: 1, c: 1, d: 1, e: { $lte: 25 } };

   var indexName = "";

   indexName = dbNodeCL.find( query1 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, idxName2 );
   indexName = dbNodeCL.find( query2 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, idxName2 );

   db.analyze( { Collection: testConf.csName + "." + testConf.clName } );

   var query3 = getOneSample( dbNode, testConf.csName, testConf.clName, idxName2 );
   var query4 = query3;
   query4.e = { $lte: 25 };

   indexName = dbNodeCL.find( query1 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, idxName2 );
   indexName = dbNodeCL.find( query2 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, idxName2 );
   indexName = dbNodeCL.find( query3 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, idxName2 );
   indexName = dbNodeCL.find( query4 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, idxName2 );

   dbNodeIXStat.update( { $set: { IndexPages: 1200 } },
      { Collection: testConf.clName, CollectionSpace: testConf.csName, Index: idxName2 } );
   dbNodeIXStat.update( { $set: { IndexPages: 1000 } },
      { Collection: testConf.clName, CollectionSpace: testConf.csName, Index: idxName1 } );
   dbNodeCLStat.update( { $set: { TotalRecords: 7000000 } },
      { Collection: testConf.clName, CollectionSpace: testConf.csName } );
   db.analyze( { Collection: testConf.csName + "." + testConf.clName, Mode: 5 } );

   indexName = dbNodeCL.find( query1 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, idxName2 );
   indexName = dbNodeCL.find( query2 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, idxName2 );
   indexName = dbNodeCL.find( query3 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, idxName2 );
   indexName = dbNodeCL.find( query4 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, idxName2 );
}
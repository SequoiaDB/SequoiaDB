/******************************************************************************
 * @Description   : seqDB-32583:多个复合索引，索引最后一个字段不同，正确选择索引
 * @Author        : tangtao
 * @CreateTime    : 2023.07.26
 * @LastEditTime  : 2023.07.26
 * @LastEditors   : tangtao
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.clName = COMMCLNAME + "_32583";

main( test );
function test ( testPara )
{
   var cl = testPara.testCL;
   var idxName1 = "index_32583_abcdefg";
   var idxName2 = "index_32583_abcdeh";

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
            f: i + j * batchSize,
            g: i + j * batchSize,
            h: i + j * batchSize
         } );
      cl.insert( data );
   }

   cl.createIndex( idxName1, { "a": 1, "b": 1, "c": 1, "d": 1, "e": 1, "f": 1, "g": 1 } );
   cl.createIndex( idxName2, { "a": 1, "b": 1, "c": 1, "d": 1, "e": 1, "h": 1 } );

   var srcGroupName = testPara.srcGroupName;
   var dbNode = db.getRG( srcGroupName ).getMaster().connect();
   var dbNodeCL = dbNode.getCS( testConf.csName ).getCL( testConf.clName );
   var dbNodeCLStat = dbNode.getCS( "SYSSTAT" ).getCL( "SYSCOLLECTIONSTAT" );
   var dbNodeIXStat = dbNode.getCS( "SYSSTAT" ).getCL( "SYSINDEXSTAT" );

   var indexName = "";
   var query1 = { a: 1000000, b: 1000000, c: 1000000, d: 1000000, e: 1000000, f: { $ne: null }, g: { $lte: 2 } };
   var query2 = { a: 1, b: 1, c: 1, d: 1, e: 1, f: { $ne: null }, g: { $lte: 2 } };
   var query3 = { a: 1, b: 1, c: 1, d: 1, e: 1, f: 1, g: 1 };

   indexName = dbNodeCL.find( query1 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, idxName1 );
   indexName = dbNodeCL.find( query2 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, idxName1 );
   indexName = dbNodeCL.find( query3 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, idxName1 );

   db.analyze( { Collection: testConf.csName + "." + testConf.clName } );

   var query4 = getOneSample( dbNode, testConf.csName, testConf.clName, idxName1 );

   indexName = dbNodeCL.find( query1 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, idxName1 );
   indexName = dbNodeCL.find( query2 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, idxName1 );
   indexName = dbNodeCL.find( query3 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, idxName1 );
   indexName = dbNodeCL.find( query4 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, idxName1 );

   dbNodeIXStat.update( { $set: { IndexPages: 1 } },
      { Collection: testConf.clName, CollectionSpace: testConf.csName, Index: idxName1 } );
   dbNodeIXStat.update( { $set: { IndexPages: 1500 } },
      { Collection: testConf.clName, CollectionSpace: testConf.csName, Index: idxName2 } );
   dbNodeCLStat.update( { $set: { TotalRecords: 7000000 } },
      { Collection: testConf.clName, CollectionSpace: testConf.csName } );
   db.analyze( { Collection: testConf.csName + "." + testConf.clName, Mode: 5 } );

   indexName = dbNodeCL.find( query1 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, idxName1 );
   indexName = dbNodeCL.find( query2 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, idxName1 );
   indexName = dbNodeCL.find( query3 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, idxName1 );
   indexName = dbNodeCL.find( query4 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, idxName1 );
}
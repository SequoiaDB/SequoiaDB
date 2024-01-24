/******************************************************************************
 * @Description   : seqDB-32584:复合索引，字段匹配最后字段顺序不同，带排序查询
 * @Author        : tangtao
 * @CreateTime    : 2023.07.26
 * @LastEditTime  : 2023.07.26
 * @LastEditors   : tangtao
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.clName = COMMCLNAME + "_32584";

main( test );
function test ( testPara )
{
   var cl = testPara.testCL;
   var idxName1 = "index_32584_abcdefg";
   var idxName2 = "index_32584_abcdegf";

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
   cl.createIndex( idxName2, { "a": 1, "b": 1, "c": 1, "d": 1, "e": 1, "g": 1, "f": 1 } );

   var srcGroupName = testPara.srcGroupName;
   var dbNode = db.getRG( srcGroupName ).getMaster().connect();
   var dbNodeCL = dbNode.getCS( testConf.csName ).getCL( testConf.clName );
   var dbNodeCLStat = dbNode.getCS( "SYSSTAT" ).getCL( "SYSCOLLECTIONSTAT" );
   var dbNodeIXStat = dbNode.getCS( "SYSSTAT" ).getCL( "SYSINDEXSTAT" );

   var indexName = "";
   var query = { a: 1, b: 1, c: 1, d: 1, e: 1, f: { $ne: null }, g: { $lte: 2 } };

   indexName = dbNodeCL.find( query ).sort( { g: -1 } ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, idxName2 );

   db.analyze( { Collection: testConf.csName + "." + testConf.clName } );

   indexName = dbNodeCL.find( query ).sort( { g: -1 } ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, idxName2 );

   dbNodeIXStat.update( { $set: { IndexPages: 1500 } },
      { Collection: testConf.clName, CollectionSpace: testConf.csName, Index: idxName1 } );
   dbNodeIXStat.update( { $set: { IndexPages: 1500 } },
      { Collection: testConf.clName, CollectionSpace: testConf.csName, Index: "abcdeh" } );
   dbNodeCLStat.update( { $set: { TotalRecords: 7000000 } },
      { Collection: testConf.clName, CollectionSpace: testConf.csName } );
   db.analyze( { Collection: testConf.csName + "." + testConf.clName, Mode: 5 } );

   indexName = dbNodeCL.find( query ).sort( { g: -1 } ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, idxName2 );
}
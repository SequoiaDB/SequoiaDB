/***************************************************************************************************
 * @Description: 复合索引选择
 * @ATCaseID: <填写 story 文档中验收用例的用例编号>
 * @Author: Zhou Hongye
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 07/20/2023 HGM           Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：
 * 测试场景：
 *    在集合上有普通索引和复合索引，对集合执行查询，查询条件为复合索引覆盖的所有字段，查看访问计划选择的索引为该复合索引
 * 测试步骤：
 *    1.创建集合
 *    2.在集合上创建字段a,b,c,d,e的复合索引，字段a,b,c,d的复合索引
 *    3.写入一定量数据
 *    4.执行查询，查询条件覆盖a,b,c,d,e
 *
 * 期望结果：
 *    访问计划选择的索引为字段a,b,c,d,e的复合索引
 *
 **************************************************************************************************/
testConf.clName = COMMCLNAME + "_compositeIndex_at_3";

main(test);
function test(testPara)
{
   db.setSessionAttr({PreferredInstance:"m"}) ;
   var cl = testPara.testCL;

   var batchSize = 10000;
   for (j = 0; j < 5; j++) {
      data = [];
      for (i = 0; i < batchSize; i++)
         data.push({
            a: i + j * batchSize,
            b: i + j * batchSize,
            c: i + j * batchSize,
            d: i + j * batchSize,
            e: i + j * batchSize,
         });
      cl.insert(data);
   }

   cl.createIndex("abcd",{"a":1,"b":1,"c":1,"d":1});
   cl.createIndex("abcde",{"a":1,"b":1,"c":1,"d":1,"e":1});

   var dbNode = selectPrimaryNode( db, testConf.csName, testConf.clName );
   var dbNodeCL = dbNode.getCS( testConf.csName ).getCL( testConf.clName );
   var dbNodeCLStat = dbNode.getCS( "SYSSTAT" ).getCL( "SYSCOLLECTIONSTAT" );
   var dbNodeIXStat = dbNode.getCS( "SYSSTAT" ).getCL( "SYSINDEXSTAT" );

   var query1 = { a:1, b:1, c:1, d:1, e:1 };
   var query2 = { a:1, b:1, c:1, d:1, e:{$lte:25} };

   var indexName = ""

   indexName = dbNodeCL.find(query1).explain().current().toObj()["IndexName"];
   assert.equal( indexName, "abcde" );
   indexName = dbNodeCL.find(query2).explain().current().toObj()["IndexName"];
   assert.equal( indexName, "abcde" );

   db.analyze({Collection: testConf.csName + "." + testConf.clName } );

   var query3 = getOneSample( dbNode, testConf.csName, testConf.clName, "abcde" );
   var query4 = query3 ;
   query4.e = {$lte:25} ;

   indexName = dbNodeCL.find(query1).explain().current().toObj()["IndexName"];
   assert.equal( indexName, "abcde" );
   indexName = dbNodeCL.find(query2).explain().current().toObj()["IndexName"];
   assert.equal( indexName, "abcde" );
   indexName = dbNodeCL.find(query3).explain().current().toObj()["IndexName"];
   assert.equal( indexName, "abcde" );
   indexName = dbNodeCL.find(query4).explain().current().toObj()["IndexName"];
   assert.equal( indexName, "abcde" );

   dbNodeIXStat.update({$set:{IndexPages:1200}},
                       {Collection:testConf.clName,CollectionSpace:testConf.csName,Index:"abcde"});
   dbNodeIXStat.update({$set:{IndexPages:1000}},
                       {Collection:testConf.clName,CollectionSpace:testConf.csName,Index:"abcd"});
   dbNodeCLStat.update({$set:{TotalRecords:7000000}},
                       {Collection:testConf.clName,CollectionSpace:testConf.csName});
   db.analyze({Collection: testConf.csName + "." + testConf.clName,Mode:5});

   indexName = dbNodeCL.find(query1).explain().current().toObj()["IndexName"];
   assert.equal( indexName, "abcde" );
   indexName = dbNodeCL.find(query2).explain().current().toObj()["IndexName"];
   assert.equal( indexName, "abcde" );
   indexName = dbNodeCL.find(query3).explain().current().toObj()["IndexName"];
   assert.equal( indexName, "abcde" );
   indexName = dbNodeCL.find(query4).explain().current().toObj()["IndexName"];
   assert.equal( indexName, "abcde" );
}
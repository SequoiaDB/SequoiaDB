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
 *    2.在集合上创建字段a,b,c,d,e,f,g的复合索引，字段a,b,c,d,e,g,f的复合索引
 *    3.写入一定量数据
 *    4.执行查询，查询条件覆盖a,b,c,d,e,f,g，a-e为等值，f,g非等值，并对g排序
 *
 * 期望结果：
 *    访问计划选择的索引为字段a,b,c,d,e,g,f的复合索引
 *
 **************************************************************************************************/
testConf.clName = COMMCLNAME + "_compositeIndex_at_5";

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
            f: i + j * batchSize,
            g: i + j * batchSize,
            h: i + j * batchSize
         });
      cl.insert(data);
   }

   cl.createIndex("abcdefg",{"a":1,"b":1,"c":1,"d":1,"e":1,"f":1,"g":1});
   cl.createIndex("abcdegf",{"a":1,"b":1,"c":1,"d":1,"e":1,"g":1,"f":1});

   var dbNode = selectPrimaryNode( db, testConf.csName, testConf.clName );
   var dbNodeCL = dbNode.getCS( testConf.csName ).getCL( testConf.clName );
   var dbNodeCLStat = dbNode.getCS( "SYSSTAT" ).getCL( "SYSCOLLECTIONSTAT" );
   var dbNodeIXStat = dbNode.getCS( "SYSSTAT" ).getCL( "SYSINDEXSTAT" );

   var indexName = "";
   var query = { a:1, b:1, c:1, d:1, e:1, f:{$ne:null}, g:{$lte:2} };

   indexName = dbNodeCL.find(query).sort({g:-1}).explain().current().toObj()["IndexName"];
   assert.equal( indexName, "abcdegf" );

   db.analyze({Collection: testConf.csName + "." + testConf.clName } );

   indexName = dbNodeCL.find(query).sort({g:-1}).explain().current().toObj()["IndexName"];
   assert.equal( indexName, "abcdegf" );

   dbNodeIXStat.update({$set:{IndexPages:1500}},
                       {Collection:testConf.clName,CollectionSpace:testConf.csName,Index:"abcdefg"});
   dbNodeIXStat.update({$set:{IndexPages:1500}},
                       {Collection:testConf.clName,CollectionSpace:testConf.csName,Index:"abcdeh"});
   dbNodeCLStat.update({$set:{TotalRecords:7000000}},
                       {Collection:testConf.clName,CollectionSpace:testConf.csName});
   db.analyze({Collection: testConf.csName + "." + testConf.clName,Mode:5});

   indexName = dbNodeCL.find(query).sort({g:-1}).explain().current().toObj()["IndexName"];
   assert.equal( indexName, "abcdegf" );
}
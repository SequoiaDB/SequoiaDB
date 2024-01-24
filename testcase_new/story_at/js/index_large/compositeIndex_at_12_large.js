/***************************************************************************************************
 * @Description: 复合索引选择
 * @ATCaseID: <填写 story 文档中验收用例的用例编号>
 * @Author: He Guoming
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
 *    2.在集合上创建字段a,b的复合索引，b的索引
 *    3.写入一定量数据
 *    4.执行查询，查询条件为{a:{$in:[100个数值]}, b:1}
 *
 * 期望结果：
 *    访问计划选择的索引为字段a,b的复合索引
 *
 **************************************************************************************************/
testConf.clName = COMMCLNAME + "_compositeIndex_at_12_large";

main(test);
function test(testPara)
{
   db.setSessionAttr({PreferredInstance:"m"}) ;
   var cl = testPara.testCL;

   var batchSize = 10000;
   for (j = 0; j < 30; j++) {
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

   cl.createIndex("acbd", {a:1,c:1,b:1,d:1}) ;
   cl.createIndex("ab", {a:1,b:1}) ;
   cl.createIndex("b", {b:1}) ;

   data=[];
   for ( i = 0; i < 1000; i++ )
   {
      data.push( i * 10 ) ;
   }

   var query1 = { a: {$in: data}, b:1 };
   var query2 = { a: {$in: data} };
   var indexName = "";
   indexName = cl.find(query1).explain().current().toObj()["IndexName"];
   assert.equal( indexName, "ab" );
   indexName = cl.find(query2).explain().current().toObj()["IndexName"];
   assert.equal( indexName, "ab" );

   db.analyze({Collection: testConf.csName + "." + testConf.clName } );

   indexName = cl.find(query1).explain().current().toObj()["IndexName"];
   assert.equal( indexName, "ab" );
   indexName = cl.find(query2).explain().current().toObj()["IndexName"];
   assert.equal( indexName, "ab" );
}
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
 *    在集合上选择时两个索引的MCV不一致
 * 测试步骤：
 *    1.创建集合
 *    2.在集合上创建字段a,c的复合索引，字段a,b,c的复合索引
 *    3.写入一定量数据
 *    4.执行查询，查询条件覆盖a，并对c排序
 *
 * 期望结果：
 *    访问计划选择的索引为字段a,c的复合索引
 *
 **************************************************************************************************/
testConf.clName = COMMCLNAME + "_compositeIndex_at_8";

main(test);
function test(testPara)
{
   db.setSessionAttr({PreferredInstance:"m"}) ;
   var cl = testPara.testCL;

   cl.createIndex("abc",{"a":1,"b":1,"c":1});
   cl.createIndex("ac",{"a":1,"c":1});

   var batchSize = 10000;
   for (j = 0; j < 5; j++) {
      data = [];
      for (i = 0; i < batchSize; i++)
         data.push({
            a: Math.round(( Math.random() * 80 )),
            b: randomString( 800 ),
            c: i,
            d: i
         });
      cl.insert(data);
   }

   db.analyze({Collection: testConf.csName + "." + testConf.clName, SampleNum:100 } );

   var dbNode = selectPrimaryNode( db, testConf.csName, testConf.clName );
   var dbNodeIXStat = dbNode.getCS( "SYSSTAT" ).getCL( "SYSINDEXSTAT" );

   var statACCur = dbNodeIXStat.aggregate(
            {$match:{CollectionSpace:testConf.csName,Collection:testConf.clName,Index:"ac","MCV.Values":{$expand:1}}},
            {$project:{"Key":"$MCV.Values.a"}},
            {$group:{_id:"$Key",Key:{$first:"$Key"},Count:{$count:"$Key"}}},
            {$match:{Count:{$gt:1}}})

   while ( statACCur.next() )
   {
      var statRecord = statACCur.current().toObj()
      println( "ab: " + JSON.stringify( statRecord ) )
      var statABCCur = dbNodeIXStat.aggregate(
            {$match:{CollectionSpace:testConf.csName,Collection:testConf.clName,Index:"abc","MCV.Values":{$expand:1}}},
            {$project:{"Key":"$MCV.Values.a"}},
            {$group:{_id:"$Key",Key:{$first:"$Key"},Count:{$count:"$Key"}}},
            {$match:{Key:{$et:statRecord["Key"]}}})
      if ( statABCCur.next() )
      {
         println( "abc: " + JSON.stringify( statABCCur.current().toObj() ) )
      }
      else
      {
         println( "abc: not found" )
      }
      db.analyze({Mode:5, Collection: testConf.csName + "." + testConf.clName})
      var indexName = cl.find({a:statRecord["Key"],d:10}).sort({c:-1}).explain().current().toObj()["IndexName"];
      assert.equal( indexName, "ac" );
   }
}
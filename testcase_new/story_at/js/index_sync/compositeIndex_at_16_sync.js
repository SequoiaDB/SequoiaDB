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
 *    在主子表集合上有多个索引，较优的索引在较后的索引槽位
 * 测试步骤：
 *    1.创建集合
 *    2.在集合上创建多个索引
 *    3.执行查询，查询条件使用较后的索引为最优
 *
 * 期望结果：
 *    访问计划选择的索引为槽位较后的索引
 *
 **************************************************************************************************/

var tempCSName = COMMCSNAME + "_compositeIndex_at_16_cs";
var mainCLName = COMMCSNAME + "_compositeIndex_at_16_main";
var subCLName = COMMCSNAME + "_compositeIndex_at_16_sub";

testConf.clName = COMMCLNAME + "_compositeIndex_at_16";

main(testWrap)

function testWrap(testPara)
{
   db.deleteConf({plancachemainclthreshold:1})
   try
   {
      var group = selectGroup(db, testConf.csName, testConf.clName);
      var cs = commCreateCS(db, tempCSName);
      var maincl = cs.createCL(mainCLName, {IsMainCL: true, ShardingKey: {a:1}});
      var subcl = cs.createCL(subCLName, {Group: group});

      maincl.attachCL(tempCSName + "." + subCLName, {LowBound:{a:-1000000000},UpBound:{a:0}})
      maincl.attachCL(testConf.csName + "." + testConf.clName, {LowBound:{a:0},UpBound:{a:1000000000}})

      maincl.createIndex('abcd',{a:1,b:1,c:1,d:1})
      maincl.createIndex('e',{e:1})
      maincl.createIndex('af',{a:1,f:1})
      maincl.createIndex('gh',{g:1,h:1})
      maincl.createIndex('ijkc',{i:1,j:1,k:1,c:1})
      maincl.createIndex('l',{l:1})
      maincl.createIndex('m',{m:1})
      maincl.createIndex('n',{n:1})
      maincl.createIndex('o',{o:1})
      maincl.createIndex('pq',{p:1,q:1})

      test(testPara);
   }
   finally
   {
      commDropCS(db, tempCSName)
      db.deleteConf({plancachemainclthreshold:1})
   }
}

function testPlan(alwaysMainCLPlan)
{
   // make the sub-collection cache plan valid
   var subcl = db.getCS(tempCSName).getCL(subCLName);
   for ( var j = 0 ; j < 10 ; ++ j )
   {
      var temp_1 = []
      for( var i = 0 ; i < 40 + j ; ++ i )
      {
         temp_1.push(i)
      }
      var temp_2 = []
      for( var i = 0 ; i < 8 ; ++ i )
      {
         temp_2.push(i)
      }
      var query = {h:"N", j:{$in:temp_2}, l:{$in:temp_1}}
      var order = {u:1}
      var indexName = subcl.find(query).sort(order).hint({"":""}).explain().current().toObj()["IndexName"]
      println("cl: " + tempCSName + "." + subCLName + " index: " + indexName)
      assert.equal( indexName, "gh" );
   }

   var cl = db.getCS(tempCSName).getCL(mainCLName);
   var temp_1 = []
   for( var i = 0 ; i < 40 ; ++ i )
   {
      temp_1.push(i)
   }
   var temp_2 = []
   for( var i = 0 ; i < 8 ; ++ i )
   {
      temp_2.push(i)
   }
   var query = {h:"N", j:{$in:temp_2},l:{$in:temp_1}}
   var order = {u:1}

   var plan = cl.find(query).sort(order).hint({"":""}).explain().current().toObj();
   for ( var i = 0 ; i < plan["SubCollections"].length ; ++ i )
   {
      clName = plan["SubCollections"][i]["Name"]
      indexName = plan["SubCollections"][i]["IndexName"]
      println("cl: " + clName + " index: " + indexName)
      if ( !alwaysMainCLPlan && clName == testConf.csName + "." + testConf.clName )
      {
         assert.equal( indexName, "l" );
      }
      else
      {
         assert.equal( indexName, "gh" );
      }
   }
}

function test(testPara)
{
   db.setSessionAttr({PreferredInstance:"m"}) ;
   var cl = db.getCS(tempCSName).getCL(mainCLName);

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
            h: i + j * batchSize,
            i: i + j * batchSize,
            j: i + j * batchSize,
            k: i + j * batchSize,
            l: i + j * batchSize,
            m: i + j * batchSize,
            n: i + j * batchSize,
            o: i + j * batchSize,
            p: i + j * batchSize,
            q: i + j * batchSize,
            r: i + j * batchSize,
            s: i + j * batchSize,
            t: i + j * batchSize,
            u: i + j * batchSize
         });
      cl.insert(data);
   }

   testPlan(false)
   db.updateConf({plancachemainclthreshold:-1})
   testPlan(false)
   db.updateConf({plancachemainclthreshold:0})
   testPlan(true)
   db.deleteConf({plancachemainclthreshold:1})

   // analyze
   db.analyze()

   testPlan(false)
   db.updateConf({plancachemainclthreshold:-1})
   testPlan(false)
   db.updateConf({plancachemainclthreshold:0})
   testPlan(true)
   db.deleteConf({plancachemainclthreshold:1})
}

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
 *    数据页多但数据记录少时，唯一键访问有走表扫描的风险
 * 测试步骤：
 *    1.创建集合
 *    2.在集合上创建唯一索引
 *    3.插入数据后删除，使记录数少，但数据页多，analyze()后执行主键查询
 *
 * 期望结果：
 *    访问计划选择的索引为唯一索引
 *
 **************************************************************************************************/

testConf.clName = COMMCLNAME + "_compositeIndex_at_17";

main(test)

function test(testPara)
{
   db.setSessionAttr({PreferredInstance:"m"}) ;
   var cl = testPara.testCL;
   cl.createIndex('ab',{a:1,b:1},true)
   for(i=0;i<20;i++)cl.insert({a:""+i*100+"123456789012345678901234567890"+i*100,b:-i})
   data=[]
   for(i=1;i<5628;i++)data.push({a:""+i+"123456789012345678901234567890"+i,b:i,c:i,d:i,e:i,f:i,g:i,h:i,i:i,j:i,k:i,m:i,n:i,o:i,p:i,q:i,r:i,s:i,t:i,u:i})
   cl.insert(data)
   cl.remove({b:{$gte:0}})
   db.analyze()
   var indexName = cl.find({a:"16001234567890123456789012345678901600",b:-16}).explain().current().toObj()["IndexName"];
   assert.equal( indexName, "ab" );
}

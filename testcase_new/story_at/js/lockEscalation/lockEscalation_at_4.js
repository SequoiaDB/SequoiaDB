/***************************************************************************************************
 * @Description: 锁升级和删除记录
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
 *    删除记录后在查询上锁升级
 * 测试步骤：
 *    1.创建集合
 *    2.灌入数据
 *    3.删除数据，再查询导致锁升级，锁升级时读取的是内存的记录
 *
 * 期望结果：
 *    删除成功，查询结果正确
 *
 **************************************************************************************************/

testConf.clName = COMMCLNAME + "_lockEscalation_at_4"

main(test);
function test(testPara)
{
   var cl = testPara.testCL

   cl.insert({_id:1,a:1})
   cl.insert({_id:2,a:2})
   cl.insert({_id:3,a:3})
   cl.insert({_id:4,a:4})

   cl.createIndex("test",{a:1});

   db.setSessionAttr({TransMaxLockNum:1, TransIsolation:2})
   db.transBegin()
   cl.remove({_id:2, a:2}, {"": "test"})
   checkLockEscalated(db, false)
   var res = commCursor2Array( cl.find().sort({a:1}).hint({"": "test"}) )
   // read id:2 in memory trigger the lock escalation
   assert.equal(res, [{_id:1,a:1},{_id:3,a:3},{_id:4,a:4}])
   checkLockEscalated(db, true)
   db.transCommit()
}
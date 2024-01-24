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
 *    在集合上锁升级和删除记录
 * 测试步骤：
 *    1.创建集合
 *    2.灌入数据
 *    3.执行批量删除导致锁升级，锁升级时读取的是磁盘的记录
 *
 * 期望结果：
 *    删除成功
 *
 **************************************************************************************************/

testConf.clName = COMMCLNAME + "_lockEscalation_at_1"

main(test);
function test(testPara)
{
   var cl = testPara.testCL

   cl.insert({_id:1,a:1})
   cl.insert({_id:2,a:1})
   cl.insert({_id:3,a:1})
   cl.insert({_id:4,a:1})

   cl.createIndex("test",{a:1});

   db.setSessionAttr({TransMaxLockNum:1})
   db.transBegin()
   cl.remove({_id:1, a:1}, {"": "test"})
   // the rule is current lock number > TransMaxLockNum, next lock acquire will escalate
   // the last remove have only 2 locks at max, so no lock escalation
   checkLockEscalated(db, false)
   cl.remove({_id:2, a:1}, {"": "test"})
   // the last remove have 3 locks at max, so trigger lock escalation
   checkLockEscalated(db, true)
   cl.remove({_id:3, a:1}, {"": "test"})
   cl.remove({_id:4, a:1}, {"": "test"})
   db.transCommit()
}
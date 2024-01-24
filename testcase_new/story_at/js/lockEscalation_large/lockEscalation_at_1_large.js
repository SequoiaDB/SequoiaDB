/***************************************************************************************************
 * @Description: 锁升级和大规模删除记录
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
 *    在大数据量集合上锁升级和大规模删除记录
 * 测试步骤：
 *    1.创建集合
 *    2.灌入数据
 *    3.执行批量删除导致锁升级
 *
 * 期望结果：
 *    删除成功
 *
 **************************************************************************************************/

testConf.clName = COMMCLNAME + "_lockEscalation_at_1_large"

main(test)

function test(testPara)
{
   var cl = testPara.testCL
   cl.insert(largeData)

   cl.createIndex("test", {ORG_CD:1, LEGAL_ENTITY_CODE:1})

   db.setSessionAttr({TransMaxLockNum:6000})
   db.transBegin()
   var cursor = cl.find()
   while ( cursor.next() )
   {
      var record = cursor.current()
      cl.remove(record, {"": "test"})
   }
   db.transCommit()
}
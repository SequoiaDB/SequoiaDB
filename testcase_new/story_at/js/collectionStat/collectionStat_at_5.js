/***************************************************************************************************
 * @Description: 验证设置会话属性是否生效
 * @ATCaseID: collectionStat_5
 * @Author: Cheng Jingjing
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who            Description
 * ========== ============== =========================================================
 * 11/21/2022 Cheng Jingjing Verify if the settings of session are take effect.
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可
 * 测试场景：
 *    以普通表为例，验证设置会话属性是否生效
 * 测试步骤：
 *    1.设置PreferredConstraint为primaryonly
 *    2.设置PreferredInstance为S
 *    3.调用getCollectionStat访问备节点查看是否报错
 * 期望结果：
 *    报错
 *
 **************************************************************************************************/
testConf.clName = COMMCLNAME + "_collectionStat_5";
testConf.useSrcGroup = true;

main(test);
function test(testPara) {
  var options = { PreferedInstance: "S", PreferredConstraint: "primaryonly" };
  db.setSessionAttr(options);
  testPara.testCL.getCollectionStat();
  // assert.tryThrow(SDB_INVALIDARG, function () {
  //   testPara.testCL.getCollectionStat();
  // });
}

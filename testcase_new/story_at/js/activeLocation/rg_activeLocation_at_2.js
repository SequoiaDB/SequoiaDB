/***************************************************************************************************
 * @Description: rg.setActiveLocation 功能验证
 * @ATCaseID: rg_activeLocation_at_2
 * @Author: Chengxi Liao
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 2/28/2023 Chengxi Liao   Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：一个包含2个节点的复制组
 * 测试场景：
 *    测试复制组在不同的 Location 前提下设置 ActiveLocation
 * 测试步骤：
 *    1. 给两个节点设置不同的位置 "location1"，"location2"
 *    2. 使用不同的参数调用rg.setActiveLocation()，查看结果
 * 期望结果：
 *    步骤2：返回/报错信息正确
 **************************************************************************************************/

testConf.skipStandAlone = true;

main(test);
function test() {
  var groupName = "group_activeLocation_2";
  var location1 = "rg_activeLocation_at_2_1";
  var location2 = "rg_activeLocation_at_2_2";
  commCheckBusinessStatus(db);

  // Step 0: get rg and node
  var rg = db.getRG(groupName);
  var nodeList = commGetGroupNodes(db, groupName);

  // Step 1: set location for two node
  setLocationForNodes(rg, nodeList.slice(0, 1), location1);
  setLocationForNodes(rg, nodeList.slice(1, 2), location2);

  // Step 2: set activeLocation for rg in different scenarios
  // old activeLocation -> new activeLocation: "" -> ""
  rg.setActiveLocation("");
  checkActiveLocation(db, groupName, "");

  // old activeLocation -> new activeLocation: "" -> "rg_activeLocation_at_2_1"
  rg.setActiveLocation(location1);
  checkActiveLocation(db, groupName, location1);

  // old activeLocation -> new activeLocation: "rg_activeLocation_at_2_1" -> "rg_activeLocation_at_2_2"
  rg.setActiveLocation(location2);
  checkActiveLocation(db, groupName, location2);

  // old activeLocation -> new activeLocation: "rg_activeLocation_at_2_2" -> "rg_activeLocation_at_2_2"
  rg.setActiveLocation(location2);
  checkActiveLocation(db, groupName, location2);

  // old activeLocation -> new activeLocation: "rg_activeLocation_at_2_2" -> "nonExistLocation"
  assert.tryThrow(SDB_INVALIDARG, function () {
    rg.setActiveLocation("nonExistLocation");
  });
  checkActiveLocation(db, groupName, location2);

  // Remove "rg_activeLocation_at_2_2" location in gropus
  setLocationForNodes(rg, nodeList.slice(1, 2), "");
  checkActiveLocation(db, groupName, "");

  // old activeLocation -> new activeLocation: "" -> "rg_activeLocation_at_2_1"
  rg.setActiveLocation(location1);
  checkActiveLocation(db, groupName, location1);

  // old activeLocation -> new activeLocation: "rg_activeLocation_at_2_1" -> ""
  rg.setActiveLocation("");
  checkActiveLocation(db, groupName, "");

  // Reset group info
  setLocationForNodes(rg, nodeList.slice(0, 2), "");
}

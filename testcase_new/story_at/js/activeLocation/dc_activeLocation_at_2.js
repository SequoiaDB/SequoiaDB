/***************************************************************************************************
 * @Description: dc.setActiveLocation 功能验证
 * @ATCaseID: dc_activeLocation_at_2
 * @Author: Chengxi Liao
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 3/01/2023 Chengxi Liao   Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：两个复制组
 * 测试场景：
 *    测试dc在不同的 Location 前提下设置 ActiveLocation
 * 测试步骤：
 *    1. 给编目和不同复制组的三个节点设置不同的位置 "location1"，"location2"
 *    2. 使用不同的参数调用dc.setActiveLocation()，查看结果
 * 期望结果：
 *    步骤2：返回/报错信息正确
 **************************************************************************************************/

testConf.skipStandAlone = true;

main(test);
function test() {
  var groupName2 = "group_activeLocation_2";
  var groupName3 = "group_activeLocation_3";
  var location1 = "dc_activeLocation_at_2_1";
  var location2 = "dc_activeLocation_at_2_2";
  commCheckBusinessStatus(db);

  // Step 0: get dc and get nodes
  var dc = db.getDC();
  var node1 = db.getRG(groupName2).getSlave();
  var node2 = db.getRG(groupName3).getSlave();
  var node3 = db.getRG(CATALOG_GROUPNAME).getSlave();

  // Step 1: set location for three groups
  node1.setLocation(location1);
  node2.setLocation(location2);
  node3.setLocation(location2);

  // Step 2: set activeLocation for dc in different scenarios
  // rg1: old activeLocation -> new activeLocation: "" -> "dc_activeLocation_at_2_1"
  // rg2: old activeLocation -> new activeLocation: "" -> "dc_activeLocation_at_2_1"
  // rg3: old activeLocation -> new activeLocation: "" -> "dc_activeLocation_at_2_1"
  assert.tryThrow(SDB_COORD_NOT_ALL_DONE, function () {
    dc.setActiveLocation(location1);
  });
  checkActiveLocation(db, groupName2, location1);
  checkActiveLocation(db, groupName3, "");
  checkActiveLocation(db, CATALOG_GROUPNAME, "");

  // rg1: old activeLocation -> new activeLocation: "dc_activeLocation_at_2_1" -> "nonExistLocation"
  // rg2: old activeLocation -> new activeLocation: "" -> "nonExistLocation"
  // rg3: old activeLocation -> new activeLocation: "" -> "nonExistLocation"
  assert.tryThrow(SDB_COORD_NOT_ALL_DONE, function () {
    dc.setActiveLocation("nonExistLocation");
  });
  checkActiveLocation(db, groupName2, location1);
  checkActiveLocation(db, groupName3, "");
  checkActiveLocation(db, CATALOG_GROUPNAME, "");

  // rg1: old activeLocation -> new activeLocation: "dc_activeLocation_at_2_1" -> "dc_activeLocation_at_2_2"
  // rg2: old activeLocation -> new activeLocation: "" -> "dc_activeLocation_at_2_2"
  // rg3: old activeLocation -> new activeLocation: "" -> "dc_activeLocation_at_2_2"
  assert.tryThrow(SDB_COORD_NOT_ALL_DONE, function () {
    dc.setActiveLocation(location2);
  });
  checkActiveLocation(db, groupName2, location1);
  checkActiveLocation(db, groupName3, location2);
  checkActiveLocation(db, CATALOG_GROUPNAME, location2);

  // rg1: old activeLocation -> new activeLocation: "dc_activeLocation_at_2_1" -> "dc_activeLocation_at_2_2"
  // rg2: old activeLocation -> new activeLocation: "dc_activeLocation_at_2_2" -> "dc_activeLocation_at_2_2"
  // rg3: old activeLocation -> new activeLocation: "dc_activeLocation_at_2_2" -> "dc_activeLocation_at_2_2"
  assert.tryThrow(SDB_COORD_NOT_ALL_DONE, function () {
    dc.setActiveLocation(location2);
  });
  checkActiveLocation(db, groupName2, location1);
  checkActiveLocation(db, groupName3, location2);
  checkActiveLocation(db, CATALOG_GROUPNAME, location2);

  // rg1: old activeLocation -> new activeLocation: "dc_activeLocation_at_2_1" -> ""
  // rg2: old activeLocation -> new activeLocation: "dc_activeLocation_at_2_2" -> ""
  // rg3: old activeLocation -> new activeLocation: "dc_activeLocation_at_2_2" -> ""
  dc.setActiveLocation("");
  checkActiveLocation(db, groupName2, "");
  checkActiveLocation(db, groupName3, "");
  checkActiveLocation(db, CATALOG_GROUPNAME, "");

  // Reset group info
  node1.setLocation("");
  node2.setLocation("");
  node3.setLocation("");
}

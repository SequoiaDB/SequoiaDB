/***************************************************************************************************
 * @Description: domain.setActiveLocation 功能验证
 * @ATCaseID: domain_activeLocation_at_2
 * @Author: Chengxi Liao
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 2/28/2023 Chengxi Liao   Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：两个复制组
 * 测试场景：
 *    测试域在不同的 Location 前提下设置 ActiveLocation
 * 测试步骤：
 *    1. 给不同域的两个节点设置不同的位置 "location1"，"location2"
 *    2. 使用不同的参数调用domain.setActiveLocation()，查看结果
 * 期望结果：
 *    步骤2：返回/报错信息正确
 **************************************************************************************************/

testConf.skipStandAlone = true;

main(test);
function test() {
  var domainName = "domain_location";
  var groupName2 = "group_activeLocation_2";
  var groupName3 = "group_activeLocation_3";
  var location1 = "domain_activeLocation_at_2_1";
  var location2 = "domain_activeLocation_at_2_2";
  commCheckBusinessStatus(db);

  // Step 0: create domain and get nodes
  var domain = db.createDomain(domainName, [groupName2]);
  var node1 = db.getRG(groupName2).getSlave();
  var node2 = db.getRG(groupName3).getSlave();

  // Step 1: set location for two groups
  node1.setLocation(location1);
  node2.setLocation(location2);

  // Step 2: set activeLocation for domain in different scenarios
  // rg1: old activeLocation -> new activeLocation: "" -> "domain_activeLocation_at_2_1"
  domain.setActiveLocation(location1);
  checkActiveLocation(db, groupName2, location1);

  // Add rg2 to domain
  domain.addGroups({ Groups: [groupName3] });

  // rg1: old activeLocation -> new activeLocation: "domain_activeLocation_at_2_1" -> "nonExistLocation"
  // rg2: old activeLocation -> new activeLocation: "" -> "nonExistLocation"
  assert.tryThrow(SDB_COORD_NOT_ALL_DONE, function () {
    domain.setActiveLocation("nonExistLocation");
  });
  checkActiveLocation(db, groupName2, location1);
  checkActiveLocation(db, groupName3, "");

  // rg1: old activeLocation -> new activeLocation: "domain_activeLocation_at_2_1" -> "domain_activeLocation_at_2_1"
  // rg2: old activeLocation -> new activeLocation: "" -> "domain_activeLocation_at_2_1"
  assert.tryThrow(SDB_COORD_NOT_ALL_DONE, function () {
    domain.setActiveLocation(location1);
  });
  checkActiveLocation(db, groupName2, location1);
  checkActiveLocation(db, groupName3, "");

  // rg1: old activeLocation -> new activeLocation: "domain_activeLocation_at_2_1" -> "domain_activeLocation_at_2_2"
  // rg2: old activeLocation -> new activeLocation: "" -> "domain_activeLocation_at_2_2"
  assert.tryThrow(SDB_COORD_NOT_ALL_DONE, function () {
    domain.setActiveLocation(location2);
  });
  checkActiveLocation(db, groupName2, location1);
  checkActiveLocation(db, groupName3, location2);

  // rg1: old activeLocation -> new activeLocation: "domain_activeLocation_at_2_1" -> ""
  // rg2: old activeLocation -> new activeLocation: "domain_activeLocation_at_2_2" -> ""
  domain.setActiveLocation("");
  checkActiveLocation(db, groupName2, "");
  checkActiveLocation(db, groupName3, "");

  // Reset group info
  node1.setLocation("");
  node2.setLocation("");
  db.dropDomain(domainName);
}

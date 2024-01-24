/***************************************************************************************************
 * @Description: startMaintenanceMode 参数校验
 * @ATCaseID: maintenanceMode_at_4.js
 * @Author: Chengxi Liao
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 7/12/2023 Chengxi Liao   Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：一个包含5个节点的复制组
 * 测试场景：
 *    测试使用 Location 开启和停止 Maintenance 模式
 * 测试步骤：
 *    1. 在复制组中对指定节点开启 Maintrnance 模式
 *    2. 在复制组中对指定节点停止 Maintenance 模式
 * 期望结果：
 *   步骤1：正常执行，参数生效
 *   步骤2：正常执行，参数生效
 **************************************************************************************************/

testConf.skipStandAlone = true;

main(test);
function test() {
  var groupName = "group_grpMode";
  var location1 = "maintenanceMode_at_2_A";
  var location2 = "maintenanceMode_at_2_B";
  commCheckBusinessStatus(db);

  // Get rg and nodes
  var rg = db.getRG(groupName);
  var nodeList = commGetGroupNodes(db, groupName);
  var replPrimary = rg.getMaster();
  var replPrimaryName = replPrimary.getHostName() + ":" + replPrimary.getServiceName();
  var slaveNodelList = getSlaveList(nodeList, replPrimaryName);

  // Set Location for nodes
  setLocationForNodes(rg, slaveNodelList.slice(0, 3), location1);
  setLocationForNodes(rg, slaveNodelList.slice(3, 5), location2);

  // Start Maintenance Mode for nodes in location1
  rg.startMaintenanceMode({ Location: location1, MinKeepTime: 5, MaxKeepTime: 10 });
  checkMaintenanceMode(db, groupName, slaveNodelList.slice(0, 3));

  // Change nodes' Location
  setLocationForNodes(rg, slaveNodelList.slice(2, 3), location2);
  checkMaintenanceMode(db, groupName, slaveNodelList.slice(0, 2));

  // Start Maintenance Mode for nodes in location2
  rg.startMaintenanceMode({ Location: location2, MinKeepTime: 5, MaxKeepTime: 10 });
  checkMaintenanceMode(db, groupName, slaveNodelList.slice(0, 5));

  // Change nodes' Location
  setLocationForNodes(rg, slaveNodelList.slice(5, 6), location2);
  checkMaintenanceMode(db, groupName, slaveNodelList.slice(0, 5));

  // Stop Maintenance Mode for nodes in location1
  rg.stopMaintenanceMode({ Location: location1 });
  checkMaintenanceMode(db, groupName, slaveNodelList.slice(2, 5));

  // Stop Maintenance Mode for all nodes
  rg.stopMaintenanceMode();
  
  // Clear environment
  rg.stopMaintenanceMode();
  setLocationForNodes(rg, slaveNodelList, "");
}

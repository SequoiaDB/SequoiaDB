/***************************************************************************************************
 * @Description: startMaintenanceMode 参数校验
 * @ATCaseID: maintenanceMode_at_3.js
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
 *    测试使用 NodeName 开启和停止 Maintenance 模式
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
  commCheckBusinessStatus(db);

  // Get rg and nodes
  var rg = db.getRG(groupName);
  var nodeList = commGetGroupNodes(db, groupName);
  var replPrimary = rg.getMaster();
  var replPrimaryName = replPrimary.getHostName() + ":" + replPrimary.getServiceName();
  var slaveNodelList = getSlaveList(nodeList, replPrimaryName);

  // Start Maintenance Mode in replica primary, this operation will take no effect
  rg.startMaintenanceMode({ NodeName: replPrimaryName, MinKeepTime: 5, MaxKeepTime: 10 });
  sleep(2);
  checkNormalGrpMode(db, groupName);

  // Start Maintenance Mode in slave node1
  rg.startMaintenanceMode({
    NodeName: slaveNodelList[0].HostName + ":" + slaveNodelList[0].svcname,
    MinKeepTime: 5,
    MaxKeepTime: 10,
  });
  checkMaintenanceMode(db, groupName, slaveNodelList.slice(0, 1));

  // Start Maintenance Mode in slave node2
  rg.startMaintenanceMode({
    NodeName: slaveNodelList[1].HostName + ":" + slaveNodelList[1].svcname,
    MinKeepTime: 5,
    MaxKeepTime: 10,
  });
  checkMaintenanceMode(db, groupName, slaveNodelList.slice(1, 2));

  // Stop Maintenance Mode in slave node1
  rg.stopMaintenanceMode({
    NodeName: slaveNodelList[0].HostName + ":" + slaveNodelList[0].svcname,
  });
  checkMaintenanceMode(db, groupName, slaveNodelList.slice(1, 2));

  // Stop Maintenance Mode in slave node2
  rg.stopMaintenanceMode({
    NodeName: slaveNodelList[1].HostName + ":" + slaveNodelList[1].svcname,
  });
  checkNormalGrpMode(db, groupName);

  // Clear environment
  rg.stopMaintenanceMode();
  checkNormalGrpMode(db, groupName);
}

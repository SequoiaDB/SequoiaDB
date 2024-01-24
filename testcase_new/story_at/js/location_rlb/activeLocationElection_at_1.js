/***************************************************************************************************
 * @Description: ActiveLocation 选举权重验证
 * @ATCaseID: activeLocationElection_at_1
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
 *    测试 ActiveLocation 和 AffinitiveLocation 的节点的选举权重。
 * 测试步骤：
 *    1. 给不同域的两个节点设置不同的位置 "location1"，"location2"
 *    2. 使用不同的参数调用domain.setActiveLocation()，查看结果
 * 期望结果：
 *    步骤2：返回/报错信息正确
 **************************************************************************************************/

testConf.skipStandAlone = true;

main(test);
function test() {
  var groupName = "group_location_rlb";
  var location = "activeLocationElection_at_1";
  var affinitiveLocation = "activeLocationElection_at_1.a";

  // Step 0: get rg, nodeList and replPrimary
  var rg = db.getRG(groupName);
  var nodeList = commGetGroupNodes(db, groupName);
  var replPrimaryName = getReplPrimaryName(rg);

  // Step 1: set location for one slave node and check location primary in location
  var locNodelList = getSlaveList(nodeList, replPrimaryName);
  var nodeName1 = locNodelList.slice(0, 1)[0];
  var nodeName2 = locNodelList.slice(1, 2)[0];
  var nodeName3 = locNodelList.slice(2, 3)[0];
  var node1 = rg.getNode(nodeName1.HostName, nodeName1.svcname);
  var node2 = rg.getNode(nodeName2.HostName, nodeName2.svcname);
  node1.setLocation(location);
  node2.setLocation(affinitiveLocation);
  rg.setActiveLocation(location);

  // Step 2: reelect replica group, check if primary is activeLocation node
  rg.reelect();
  checkNodeIsReplPrimary(rg, nodeName1.NodeID, 30);

  // Relect again
  rg.reelect();
  checkNodeIsReplPrimary(rg, nodeName1.NodeID, 30);

  // Stop primary, check affinitiveLocation
  node1.stop();
  sleep(14000);
  checkNodeIsReplPrimary(rg, nodeName2.NodeID, 30);

  // Relect again
  rg.reelect();
  checkNodeIsReplPrimary(rg, nodeName2.NodeID, 30);

  // Reelect to given node
  rg.reelect({ NodeID: nodeName3.NodeID });
  checkNodeIsReplPrimary(rg, nodeName3.NodeID, 30);

  // Remove location info
  setLocationForNodes(rg, locNodelList, "");
  rg.start();
}

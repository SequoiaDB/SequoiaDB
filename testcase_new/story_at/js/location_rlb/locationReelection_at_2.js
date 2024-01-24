/***************************************************************************************************
 * @Description: cls 多节点单位置集重新选主
 * @ATCaseID: locationReelection_at_2
 * @Author: Chengxi Liao
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 11/28/2022 Chengxi Liao  Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：一个包含四个节点的复制组
 * 测试场景：
 *    测试多节点单位置集的重新选主
 * 测试步骤：
 *    1. 给同一复制组的三个复制组备节点设置相同的位置 "A"，查看位置集内是否有主节点产生
 *    2. 使用不同的参数调用 rg.reelectLocation()，查看结果
 *        2.1 在 60s 内执行 rg.reelectLocation()，查看结果
 *        2.2 使用给定的 NodeID 执行 rg.reelectLocation()，查看结果
 *        2.3 给复制组主节点设置 Location，并使用该主节点的 HostName 和 ServiceName 执行 rg.reelectLocation()，查看结果
 *    3. 当位置集和复制组的主节点是同一个节点时，执行 rg.reelectLocation()，查看结果
 *    4. 停止两个备节点，执行 rg.reelectLocation()，查看结果
 * 期望结果：
 *    步骤1：节点为当前位置集的主节点
 *    步骤2：Location 存在主节点
 *        步骤2.1：Location 存在主节点，且与选举前的节点不同
 *        步骤2.2：Location 存在主节点，且主节点与给定的 NodeID 节点相同
 *        步骤2.3：Location 存在主节点，且主节点与复制组的主节点相同
 *    步骤3：Location 的主节点不变，仍为复制组的主节点
 *    步骤4：报错节点非位置集主节点
 *
 * 说明：Location 选举周期的最长时间为 17s，使用两个选举周期来判断超时。
 **************************************************************************************************/

testConf.skipStandAlone = true;

main(test);
function test() {
  var groupName = "group_location_rlb";
  var location = "location_locatioReelection_at_2";

  // Step 0: create a group contains one node
  var rg = db.getRG(groupName);
  var nodeList = commGetGroupNodes(db, groupName);
  var replPrimary = rg.getMaster();
  var replPrimaryName = replPrimary.getHostName() + ":" + replPrimary.getServiceName();

  // Step 1: set location for all node except replica group's primary and check location primary in location
  var locNodelList = getSlaveList(nodeList, replPrimaryName);
  setLocationForNodes(rg, locNodelList, location);
  var primary = checkAndGetLocationHasPrimary(db, groupName, location, 34);

  // Step 2: use different argument in rg.reelectLocation and check the result
  // Reelect location in 60s
  rg.reelectLocation(location, { Seconds: 60 });
  var primary1 = checkAndGetLocationHasPrimary(db, groupName, location, 1);
  assert.notEqual(primary, primary1, "Reelect location didn't change primary");

  // Reelect location using given NodeID
  var slaveList = getSlaveList(locNodelList, primary1);
  rg.reelectLocation(location, { NodeID: slaveList[0].NodeID, Seconds: 60 });
  var primary2 = checkAndGetLocationHasPrimary(db, groupName, location, 1);
  assert.equal(
    primary2,
    slaveList[0].HostName + ":" + slaveList[0].svcname,
    "Reelect location didn't change to given primary"
  );

  // Set location for replica group's primary and reelect location to this node using given HostName and ServiceName
  replPrimary.setLocation(location);
  rg.reelectLocation(location, {
    HostName: replPrimary.getHostName(),
    ServiceName: replPrimary.getServiceName(),
  });
  var primary3 = checkAndGetLocationHasPrimary(db, groupName, location, 1);
  assert.equal(
    primary3,
    replPrimary.getHostName() + ":" + replPrimary.getServiceName(),
    "Reelect location didn't change to given primary"
  );

  // Step 3: Reelect location if location primary and replica group's primary are the same
  assert.tryThrow(SDB_OPERATION_CONFLICT, function () {
    rg.reelectLocation(location);
  });
  var primary4 = checkAndGetLocationHasPrimary(db, groupName, location, 1);
  assert.equal(primary3, primary4, "Reelect location didn't change to given primary");

  assert.tryThrow(SDB_OPERATION_CONFLICT, function () {
    rg.reelectLocation(location, { NodeID: slaveList[0].NodeID });
  });
  var primary5 = checkAndGetLocationHasPrimary(db, groupName, location, 1);
  assert.equal(primary4, primary5, "Reelect location didn't change to given primary");

  // Step 4: Stop 2 of group nodes and check if locationReelect can be execute
  var node1 = rg.getNode(slaveList[0].HostName, slaveList[0].svcname);
  node1.stop();
  var node2 = rg.getNode(slaveList[1].HostName, slaveList[1].svcname);
  node2.stop();
  sleep(11000);
  assert.tryThrow(SDB_CLS_NOT_LOCATION_PRIMARY, function () {
    rg.reelectLocation(location);
  });

  // Reset group info
  rg.start();
  // removeDataGroup(db, groupName);
}

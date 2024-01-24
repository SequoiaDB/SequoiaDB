import("../lib/main.js");

/******************************************************************************
 * @description: 将节点加入当前复制组
 * @param {string} hostName  // 机器的主机名
 * @param {string} port  // 节点的端口号
 * @param {json} option  // 设置是否保留新加节点原有的数据
 ******************************************************************************/
function detachNode(rg, hostName, port, option) {
  try {
    rg.detachNode(hostName, port, option);
  } catch (e) {
    if (e != SDBCM_NODE_NOTEXISTED) {
      throw e;
    }
  }
}

/******************************************************************************
 * @description: 将节点加入当前复制组
 * @param {string} hostName  // 机器的主机名
 * @param {string} port  // 节点的端口号
 * @param {json} option  // 设置是否保留新加节点原有的数据
 ******************************************************************************/
function attachNode(rg, hostName, port, option) {
  try {
    rg.attachNode(hostName, port, option);
  } catch (e) {
    if (e != SDBCM_NODE_NOTEXISTED) {
      throw e;
    }
  }
  rg.start();
}

/******************************************************************************
 * @description: Check if the node is location primary
 * @param {string} groupName
 * @param {string} nodeName
 * @param {string} location
 * @param {int} seconds // Max wait time
 ******************************************************************************/
function checkNodeIsLocationPrimary(db, groupName, nodeName, location, seconds) {
  while (seconds-- > 0) {
    sleep(1000);
    var primary = getLocationPrimary(db, groupName, location);
    if (primary === nodeName) {
      break;
    }
  }
  assert.equal(
    nodeName,
    primary,
    "Node[" + nodeName + "] is not location primary[" + primary + "]"
  );
}

/******************************************************************************
 * @description: Check if the location has primary
 * @param {string} groupName
 * @param {string} location
 * @param {int} seconds // Max wait time
 * @return {string} primary // if location has primary, return it
 ******************************************************************************/
function checkAndGetLocationHasPrimary(db, groupName, location, seconds) {
  while (seconds-- > 0) {
    sleep(1000);
    var primary = getLocationPrimary(db, groupName, location);
    if (primary !== "") {
      break;
    }
  }
  assert.notEqual("", primary, "Location[" + location + "] doesn't have primary");
  return primary;
}

/******************************************************************************
 * @description: Check if the location doesn't have primary
 * @param {string} groupName
 * @param {string} location
 * @param {int} seconds // Max wait time
 ******************************************************************************/
function checkLocationHasNoPrimary(db, groupName, location, seconds) {
  while (seconds-- > 0) {
    sleep(1000);
    var primary = getLocationPrimary(db, groupName, location);
    if (primary === "") {
      break;
    }
  }
  assert.equal("", primary, "Location[" + location + "] still have primary");
}

/******************************************************************************
 * @description: Get location's primary node name
 * @param {string} groupName
 * @param {string} location
 * @return {string} locationPrimary  // if location has primary, return primary; else return ""
 ******************************************************************************/
function getLocationPrimary(db, groupName, location) {
  var groupObj = db.getRG(groupName).getDetailObj().toObj();

  var locations = groupObj.Locations;
  var groupInfo = groupObj.Group;

  // Get location's primary in SYSCAT.SYSNODES
  var primaryNodeID = 0;
  for (var i in locations) {
    var locationItem = locations[i];
    if (location === locationItem["Location"]) {
      if ("PrimaryNode" in locationItem) {
        primaryNodeID = locationItem["PrimaryNode"];
      } else {
        break;
      }
    }
  }
  if (primaryNodeID === 0) {
    return "";
  }

  var hostName, serviceName;
  for (var i in groupInfo) {
    var nodeItem = groupInfo[i];
    if (primaryNodeID === nodeItem["NodeID"]) {
      hostName = nodeItem["HostName"];
      var services = nodeItem["Service"];
      for (var j in services) {
        var service = services[j];
        if (0 === service["Type"]) {
          serviceName = service["Name"];
        }
      }
    }
  }
  var nodeName1 = hostName + ":" + serviceName;

  // Get location's primary in SDB_SNAP_SYSTEM
  var nodeName2 = "";
  var cursor = db.snapshot(SDB_SNAP_SYSTEM, { RawData: true, GroupName: groupName });
  while (cursor.next()) {
    var snapshotObj = cursor.current().toObj();
    if ("ErrNodes" in snapshotObj) {
      continue;
    }
    if (location === snapshotObj.Location) {
      if (true === snapshotObj.IsLocationPrimary) {
        nodeName2 = snapshotObj.NodeName;
        break;
      }
    }
  }
  if (nodeName2 === "") {
    return "";
  }

  // Get location's primary in SDB_SNAP_DATABASE
  var nodeName3 = "";
  var cursor = db.snapshot(SDB_SNAP_DATABASE, { RawData: true, GroupName: groupName });
  while (cursor.next()) {
    var snapshotObj = cursor.current().toObj();
    if ("ErrNodes" in snapshotObj) {
      continue;
    }
    if (location === snapshotObj.Location) {
      if (true === snapshotObj.IsLocationPrimary) {
        nodeName3 = snapshotObj.NodeName;
        break;
      }
    }
  }
  if (nodeName3 === "") {
    return "";
  }

  assert.equal(
    nodeName1,
    nodeName2,
    "Location[" +
      location +
      "] primary in SYSCAT.SYSNODES[" +
      nodeName1 +
      "] is not equal to in SDB_SNAP_SYSTEM[" +
      nodeName2 +
      "]"
  );
  assert.equal(
    nodeName1,
    nodeName3,
    "Location[" +
      location +
      "] primary in SYSCAT.SYSNODES[" +
      nodeName1 +
      "] is not equal to in SDB_SNAP_DATABASE[" +
      nodeName3 +
      "]"
  );

  return nodeName1;
}

/******************************************************************************
 * @description: Create a data group with n nodes
 * @param {string} groupName
 * @param {int} nodeNum
 ******************************************************************************/
function createADataGroupWithNNodes(db, groupName, nodeNum) {
  var n = nodeNum;
  var hostName = commGetGroups(db)[0][1].HostName;
  var dataRG = db.createRG(groupName);
  while (n > 0) {
    var port = parseInt(RSRVPORTBEGIN) + n * 10;
    dataRG.createNode(hostName, port, RSRVNODEDIR + "/" + port, { diaglevel: 5 });
    n--;
  }
  dataRG.start();
}

/******************************************************************************
 * @description: Create data groups with n nodes
 * @param {list} groupNameLst
 * @param {list} nodeNumLst
 ******************************************************************************/
function createDataGroupsWithNNodes(db, groupNameLst, nodeNumLst) {
  assert.equal(groupNameLst.length, nodeNumLst.length);
  var hostName = commGetGroups(db)[0][1].HostName;
  for (var idx = 0; idx < nodeNumLst.length; idx++) {
    var dataRG = db.createRG(groupNameLst[idx]);
    var n = nodeNumLst[idx];
    while (n > 0) {
      var port = parseInt(RSRVPORTBEGIN) + idx * 100 + n * 10;
      dataRG.createNode(hostName, port, RSRVNODEDIR + "/" + port, { diaglevel: 5 });
      n--;
    }
    dataRG.start();
    commCheckBusinessStatus(db);
  }
}

/******************************************************************************
 * @description: Remove a data group
 * @param {string} groupName
 ******************************************************************************/
function removeDataGroup(db, groupName) {
  try {
    db.removeRG(groupName);
  } catch (e) {
    if (e != SDB_CLS_GRP_NOT_EXIST) {
      throw new Error(e);
    }
  }
}

/******************************************************************************
 * @description: Remove data groups
 * @param {list} groupNameLst
 ******************************************************************************/
function removeDataGroups(db, groupNameLst) {
  var n = groupNameLst.length;
  while (n > 0) {
    try {
      db.removeRG(groupNameLst[n - 1]);
    } catch (e) {
      if (e != SDB_CLS_GRP_NOT_EXIST) {
        throw new Error(e);
      }
    }
    n--;
  }
}

/******************************************************************************
 * @description: Set location for given node list
 * @param {array} nodeList
 * @param {string} location
 ******************************************************************************/
function setLocationForNodes(rg, nodeList, location) {
  for (var i in nodeList) {
    var nodeInfo = nodeList[i];
    var node = rg.getNode(nodeInfo.HostName, nodeInfo.svcname);
    node.setLocation(location);
  }
}

/******************************************************************************
 * @description: Clear location for given node list
 * @param {array} nodeList
 * @param {string} location
 ******************************************************************************/
function clearLocationForNodes(rg, nodeList) {
  for (var i in nodeList) {
    var nodeInfo = nodeList[i];
    var node = rg.getNode(nodeInfo.HostName, nodeInfo.svcname);
    node.setLocation("");
  }
}

/******************************************************************************
 * @description: Get location slave node list
 * @param {array} nodeList // group node list
 * @param {string} primary // group primary node
 * @return {array} slaveList // group node list without primary
 ******************************************************************************/
function getSlaveList(nodeList, primary) {
  var slaveList = [];
  for (var i in nodeList) {
    var node = nodeList[i];
    var nodeName = node.HostName + ":" + node.svcname;
    if (nodeName != primary) {
      slaveList.push(node);
    }
  }
  return slaveList;
}

/******************************************************************************
 * @description: Use kill -9 to node list
 * @param {string} nodeList
 ******************************************************************************/
function killNodes(nodeList) {
  for (var i in nodeList) {
    var node = nodeList[i];
    var remote = new Remote(node.HostName, CMSVCNAME);
    var cmd = remote.getCmd();
    cmd.run(
      "ps -ef | grep sequoiadb | grep -v grep | grep " +
        node.svcname +
        " | awk '{print $2}' | xargs kill -9"
    );
  }
}

/******************************************************************************
 * @description: 获取复制组节点的详细信息进行校验
 * @param {string} node  // 节点名
 * @param {string} expLocation  // 需要校验的location
 ******************************************************************************/
function checkNodeLocation(node, expLocation) {
  var nodeObj = node.getDetailObj().toObj();
  actLocation = nodeObj.Location;
  assert.equal(
    actLocation,
    expLocation,
    "Node's location[" + actLocation + "] is not equal to[" + expLocation + "] "
  );
}

/******************************************************************************
 * @description: check sync source
 * @param {array} node
 * @param {string} expectNodeID
 ******************************************************************************/
function checkPeerNodeID(db, node, expectNodeID) {
  var selectOk = false;
  nodeName = node.HostName + ":" + node.svcname;
  cursor = db.snapshot(SDB_SNAP_SESSIONS, { Type: "ReplAgent", NodeName: nodeName });
  while (cursor.next()) {
    snapshotObj = cursor.current().toObj();
    sessionName = snapshotObj.Name;
    wordArrTmp = sessionName.split(/,|:|[(]|[)]/);
    wordArr = [];
    for (var i = 0; i < wordArrTmp.length; i++) {
      if (wordArrTmp[i] != "") {
        wordArr.push(wordArrTmp[i]);
      }
    }
    if (wordArr[1] == "Sync-Source") {
      if (wordArr[7] == wordArr[7]) {
        selectOk = true;
      }
    }
  }
  assert.equal(selectOk, true);
}

/******************************************************************************
 * @description: Get group's ActiveLocation
 * @param {string} groupName
 * @return {string} group's ActiveLocation  // if group has ActiveLocation, return ActiveLocation; else return ""
 ******************************************************************************/
function getActiveLocation(db, groupName) {
  var activeLocation = db.getRG(groupName).getDetailObj().toObj().ActiveLocation;

  if (activeLocation == undefined) {
    return "";
  } else {
    return activeLocation;
  }
}

/******************************************************************************
 * @description: Check if the group's activeLocation is the same as given
 * @param {string} groupName
 * @param {string} activeLocation
 ******************************************************************************/
function checkActiveLocation(db, groupName, activeLocation) {
  var _activeLocation = getActiveLocation(db, groupName);

  assert.equal(
    _activeLocation,
    activeLocation,
    "ActiveLocation[" + _activeLocation + "] in SYSCAT.SYSNODES is not equal to" + activeLocation
  );
}

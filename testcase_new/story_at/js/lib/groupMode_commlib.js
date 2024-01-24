import("./main.js");

/******************************************************************************
 * @description: Stop nodes
 * @param {array} nodeList
 ******************************************************************************/
function stopNodes(rg, nodeList) {
  for (var i in nodeList) {
    var nodeItem = nodeList[i];
    var node = rg.getNode(nodeItem.HostName, nodeItem.svcname);
    node.stop();
  }
}

/******************************************************************************
 * @description: Check if the group started critical mode successfully
 * @param {string} groupName
 * @param {string} location
 ******************************************************************************/
function checkCriticalLocationMode(db, groupName, location) {
  var rg = db.getRG(groupName);
  var replPrimary = rg.getMaster();
  var groupObj = rg.getDetailObj().toObj();

  // Check primary
  checkNodeLocation(replPrimary, location);

  // Check critical mode in groupObj
  var grpMode = groupObj.GroupMode;
  assert.equal(grpMode, "critical", "GroupMode is not critical");

  // Check critical mode in grpModeObj
  var grpModeObj = db.list(SDB_LIST_GROUPMODES, { GroupID: groupObj.GroupID }).current().toObj();
  assert.equal(grpModeObj.GroupMode, "critical", "GroupMode is not critical");

  // Check critical properties in grpModeObj
  var property = grpModeObj.Properties[0];
  assert.equal(
    property.Location,
    location,
    "Actucal location[" + property.Location + "] is not[" + location + "]"
  );
}

/******************************************************************************
 * @description: Check if the group started critical mode successfully
 * @param {string} groupName
 * @param {string} nodeName
 ******************************************************************************/
function checkCriticalNodeMode(db, groupName, nodeName) {
  var rg = db.getRG(groupName);
  var replPrimary = rg.getMaster();
  var replPrimaryName = replPrimary.getHostName() + ":" + replPrimary.getServiceName();

  var groupObj = rg.getDetailObj().toObj();
  var primaryNodeID = groupObj.PrimaryNode;

  assert.equal(
    replPrimaryName,
    nodeName,
    "Primary[" + replPrimaryName + "] is not nodeName[" + nodeName + "]"
  );

  // Check critical mode in groupObj
  var grpMode = groupObj.GroupMode;
  assert.equal(grpMode, "critical", "GroupMode is not critical");

  // Check critical mode in grpModeObj
  var grpModeObj = db.list(SDB_LIST_GROUPMODES, { GroupID: groupObj.GroupID }).current().toObj();
  assert.equal(grpModeObj.GroupMode, "critical", "GroupMode is not critical");

  // Check critical properties in grpModeObj
  var property = grpModeObj.Properties[0];
  assert.equal(
    property.NodeID,
    primaryNodeID,
    "Actucal node[" + property.NodeID + "] is not[" + primaryNodeID + "]"
  );
}

/******************************************************************************
 * @description: Check if the group started maintenance mode successfully
 * @param {string} groupName
 * @param {string} nodeList
 ******************************************************************************/
function checkMaintenanceMode(db, groupName, nodeList) {
  var rg = db.getRG(groupName);
  var groupObj = rg.getDetailObj().toObj();

  // Check maintenance mode
  var grpMode = groupObj.GroupMode;
  assert.equal(grpMode, "maintenance", "GroupMode is not maintenance");

  // Check maintenance mode in grpModeObj
  var grpModeObj = db.list(SDB_LIST_GROUPMODES, { GroupID: groupObj.GroupID }).current().toObj();
  assert.equal(grpModeObj.GroupMode, "maintenance", "GroupMode is not maintenance");

  var properties = grpModeObj.Properties;

  // Check maintenance properties in grpModeObj
  for (var i in nodeList) {
    var match = false;
    var nodeItem = nodeList[i];
    for (var j in properties) {
      var property = properties[j];
      if (property.NodeID == nodeItem.NodeID) {
        match = true;
        break;
      }
    }
    assert.equal(match, true, "Missing node[" + nodeItem.NodeID + "]");
  }
}

/******************************************************************************
 * @description: Check if the group is in normal grpMode
 * @param {string} groupName
 * @param {string} location
 ******************************************************************************/
function checkNormalGrpMode(db, groupName) {
  var rg = db.getRG(groupName);
  var groupObj = rg.getDetailObj().toObj();

  // Check critical mode
  var grpMode = groupObj.GroupMode;
  assert.equal(grpMode, undefined, "GroupMode is not undefined");

  var grpModeObj = db.list(SDB_LIST_GROUPMODES, { GroupID: groupObj.GroupID }).current().toObj();
  if (grpModeObj.Properties != undefined) {
    assert.equal(grpModeObj.GroupMode, undefined, "GroupMode is not undefined");
    assert.equal(grpModeObj.Properties[0], undefined, "Properties is not undefined");
  }
}

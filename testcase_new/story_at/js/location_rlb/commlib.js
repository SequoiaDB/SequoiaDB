import("../lib/basic_operation/commlib.js");
import("../lib/location_commlib.js");

function getReplPrimaryName(rg) {
  var replPrimary = rg.getMaster();
  var replPrimaryName = replPrimary.getHostName() + ":" + replPrimary.getServiceName();
  return replPrimaryName;
}

function getReplPrimaryNodeID(rg) {
  var nodeID = rg.getDetailObj().toObj().PrimaryNode;
  return nodeID;
}

function checkNodeIsReplPrimary(rg, nodeID, seconds) {
  while (seconds-- > 0) {
    sleep(1000);
    var primary = getReplPrimaryNodeID(rg);
    if (primary === nodeID) {
      break;
    }
  }
  assert.equal(
    nodeID,
    primary,
    "Node[" + nodeID + "] is not replica group primary[" + primary + "]"
  );
}

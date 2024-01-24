import("../lib/basic_operation/commlib.js");
import("../lib/location_commlib.js");
import("../lib/groupMode_commlib.js");

function getLSNVersion(db, groupName) {
  var cursor = db.snapshot(SDB_SNAP_SYSTEM, { RawData: true, GroupName: groupName });
  while (cursor.next()) {
    var snapshotObj = cursor.current().toObj();
    if (snapshotObj.IsPrimary) {
      return snapshotObj.CurrentLSN.Version;
    }
  }
}

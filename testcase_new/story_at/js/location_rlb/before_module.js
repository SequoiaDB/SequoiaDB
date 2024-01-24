import( "./commlib.js" );
var nodeNum = 4;
var groupName = "group_location_rlb";

// If existed group, remove
removeDataGroup(db, groupName);

createADataGroupWithNNodes(db, groupName, nodeNum);
commCheckBusinessStatus(db);
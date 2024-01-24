import( "./commlib.js" );
var nodeNum = 3;
var groupName = "save_rollback_log_rlb";

// If existed group, remove
removeDataGroup(db, groupName);

createADataGroupWithNNodes(db, groupName, nodeNum);
commCheckBusinessStatus(db);
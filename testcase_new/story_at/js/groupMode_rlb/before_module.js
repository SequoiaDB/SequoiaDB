import("./commlib.js");
var groupName = "group_grpMode";
var groupNameLst = [groupName];
var nodeNumLst = [7];

// If existed group and domain, remove
removeDataGroups(db, groupNameLst);

createDataGroupsWithNNodes(db, groupNameLst, nodeNumLst);

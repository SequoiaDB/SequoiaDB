import("./commlib.js");
var groupName2 = "group_activeLocation_2";
var groupName3 = "group_activeLocation_3";
var groupNameLst = [groupName2, groupName3];
var nodeNumLst = [2, 3];

// If existed group and domain, remove
removeDataGroups(db, groupNameLst);

createDataGroupsWithNNodes(db, groupNameLst, nodeNumLst);
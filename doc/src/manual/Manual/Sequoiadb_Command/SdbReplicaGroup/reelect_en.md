
##NAME##

reelect - Reelect the master node in the replica group.

##SYNOPSIS##

**rg.reelect([options])**

##CATEGORY##

Replica Group

##DESCRIPTION##

Reelect the master node in the replica group.

##PARAMETERS##

* `options` ( *json object*)

    Parameter collection, can be the following options:

    1. `Seconds` ( *int* ): Reelection start in how many seconds.

    2. `NodeID` ( *int* ): Node ID of the expected primary node.

    3. `HostName` ( *string* ): Host name of the expected primary node.

    4. `ServiceName` ( *string* ): Service name of the expected primary node.

**Note:**

> 1. Returning timeout error means that the reelection hasn't completed during the time we set. You can wait for several seconds until the catalog node asynchronous update complete, then using [db.listReplicaGroups()](reference/Sequoiadb_command/Sdb/listReplicaGroups.md) to view the result.

> 2. The reelection can only be started when there is a master node in the replica group.

> 3. When NodeID is used, will ignore HostName and ServiceName.

> 4. When no specific NodeID or ServiceName is specified, if more than one node can be selected as the primary node, the matching rule of the election is : LSN of the node > node weight > NodeID. The node with the largest LSN is selected. If LSN is consistent, the node with the largest weight value is selected, if the weight value is consistent, the node with the largest ID value is selected.

> 5. The surviving nodes in a replication group need to account for at least half of the total number of nodes.

> 6. The specific description of the reelection can refer to the [reelection mechanism](infrastructure/replication/vote.md)


##RETURN VALUE##

There is no return value. On error, exception will be thrown.

##ERRORS##

The exceptions of `reelect()` are as below:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -104 | SDB_CLS_NOT_PRIMARY | Primary node does not exit | Check if the current replicaGroup has a node with "isPrimary" being "true". Start the node if there is a node that is not started in the current replicaGroup. |


When error happens, use [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)
to get the error message or use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)
to get the error code. See [troubleshooting](manual/FAQ/faq_sdb.md) for
more details.

##EXAMPLES##

1. Conduct the reelection in 60s with the group of 'datagroup1'

	```lang-javascript
	> var rg = db.getRG("datagroup1") 
	> rg.reelect({Seconds:60})
	```

##NAME##

reelectLocation - reelect the primary node in the position set

##SYNOPSIS##

**rg.reelectLocation(\<location\>, [options])**

##CATEGORY##

SdbReplicaGroup

##DESCRIPTION##

This function is used to reelect the primary node of the specified position set in the current replication group.

##PARAMETERS##

- location ( *string, required* )

    Location information.

- options ( *object, optional* )

    Set the matching conditions of the primary node through the parameter "options":

    - Seconds ( *number* ): Election timeout, the default value is 30, the unit is second.

        Format: `Seconds: 50`

    - NodeID ( *number* ): The node ID of the desired primary node.

        Format: `NodeID: 1000`

    - HostName ( *string* ): The hostname of the desired primary node.

        If the parameter "NodeID" is specified, this parameter will not take effect.

        Format: `HostName: "hostname"`

    - ServiceName ( *string* ): The service name of the desired primary node.

        If the parameter "NodeID" is specified, this parameter will not take effect.

        Format: `ServiceName: "11820"`

>**Note:**
>
> - When multiple nodes are matched, the primary node will be randomly selected among the matched nodes.
> - If the parameters "NodeID", "HostName" and "ServiceName" are not specified, the system will automatically match nodes according to [Election Mechanism][Replication].
> - If the parameters "HostName" and "ServiceName" are specified at the same time, when the parameters are in effect, nodes that meet both conditions will be matched.

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `reelectLocation()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ---------- | ---------- | ----------- | -------- |
| -6 | SDB_INVALIDARG | Parameter type error. | Check whether the parameter type is correct. |
| -259 | SDB_OUT_OF_BOUND | Required parameters not specified. | Check whether there are required parameters missing. |
| -334 | SDB_OPERATION_CONFLICT | The value of the parameter "location" is the primary location set. | Currently, the reelection operation is not supported in the primary location set, users need to check whether the value of the parameter "location" is correct.  |
| -395 | SDB_CLS_NOT_LOCATION_PRIMARY | The location set is missing a primary node. | Check the node status of the current location set to ensure that the number of available nodes exceeds half of the total number of nodes in the location set. | 

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.6.1 and above

##EXAMPLES##

1. Perform a reelection operation on the location set "GuangZhou" under the replication group "group1", and set the node with NodeID "1000" as the primary node.

    ```lang-javascript
    > var rg = db.getRG("group1")
    > rg.reelectLocation("GuangZhou", {NodeID: 1000})
    ```

2. View the primary node ID corresponding to the current location set.

    ```lang-javascript
    > db.list(SDB_LIST_GROUPS, {"GroupName": "group1"}, {"Locations.Location": null, "Locations.PrimaryNode": null})
    {
      "Locations": [
        {
          "Location": "GuangZhou",
          "PrimaryNode": 1000
        }
      ]
    }
    ```

[^_^]:
     Links
[Replication]:manual/Distributed_Engine/Architecture/Replication/election.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
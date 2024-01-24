##NAME##

forceStepUp - force the standby node to be upgraded to the primary node 

##SYNOPSIS##

**db.forceStepUp([options])**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to forcibly promote the standby node to the primary node in a replication group that is not eligible for election. Before upgrading, make sure that the LSN of the target node is the maximum value in the group. If a node with a smaller LSN is forcibly promoted to master, data will be rolled back. Users can obtain node LSN information through [Node Health Detection Snapshot][SDB_SNAP_HEALTH].

>**Note:**
>
> This function is only supported in catalog replication groups.

##PARAMETERS##

options ( *object, optional* )

Modify the duration of the primary node through the parameter "options":

- Seconds ( *number* ): The duration of the forced upgrade to the master node, in seconds, the default value is 120.

    When the specified time is exceeded, the replication group will be re-elected according to the election rule.

    Format: `Seconds: 300`

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.4 and above

##EXAMPLES##

1. Connect to catalog node 11800.

    ```lang-javascript
    > var cata = new Sdb("localhost", 11800)
    ```

    >**Note:**
    >
    > If the catalog node cannot be connected, the node parameter "auth" needs to be configured to false. For the configurationmethod, can refer to [Parameter Configuration][parameter].

2. Forcibly promote catalog node 11800 to master with a specified duration of 300 seconds.

    ```lang-javascript
    > cata.forceStepUp({Seconds: 300})
    ```


[^_^]:
     Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[SDB_SNAP_HEALTH]:manual/Manual/Snapshot/SDB_SNAP_HEALTH.md
[parameter]:manual/Distributed_Engine/Maintainance/Database_Configuration/configuration_parameters.md
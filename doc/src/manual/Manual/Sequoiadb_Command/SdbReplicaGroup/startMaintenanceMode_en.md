##NAME##

startMaintenanceMode - start Maintenance mode in the current replication group

##SYNOPSIS##

**rg.startMaintenanceMode(\<options\>)**

##CATEGORY##

SdbReplicaGroup

##DESCRIPTION##

This function is used to start the Maintenance mode for the given nodes in a replication group. The nodes in Maintenance mode do not participate in replica group elections as well as the caculation of replSize.

> **Note:**
>
> If all nodes in replica group are effective in Maintenance mode, the replica group will be unavailable and there is no parimary node in this group.

##PARAMETERS##

options ( *object, required* )

Specify the Maintenance mode attributes through the parameter "options":

- NodeName ( *string* ): The effective node in Maintenance mode.

    The specified node needs to exist in the current replication group.

    Format: `NodeName:"sdbserver:11820"`

    > **Note:**  
    >
    > The primary node of replica group cann't be effective node of Maintenance mode.

- Location ( *string* ): The effective location in Maintenance mode.

    - The specified location needs to exist in the current replication group.
    - This parameter takes effect only when parameter "NodeName" is not specified.

    Format: `Location: "GuangZhou"`

- MinKeepTime ( *number* ): Minimum keep time for Maintenance mode. The value range is (0, 10080], and the unit is minutes.

    Format: `MinKeepTime: 100`

- MaxKeepTime ( *number* ): Maximum keep time for Maintenance mode. The value range is (0, 10080], and the unit is minutes.

    Format: `MaxKeepTime: 1000`

    > **Note:**
    >
    > For parameters "MinKeepTime" and "MaxKeepTime":
    > - The value of "MinKeepTime" should be less than "MaxKeepTime".
    > - After successfully starting the Maintenance mode, the replication group will remain in the Maintenance mode until the time specified by "MinKeepTime" is reached. During the period from "MinKeepTime" to "MaxKeepTime", if most nodes in the replication group are normal, the Maintenance mode will be automatically released. After the time specified by "MaxKeepTime" expires, the replication group will forcibly cancel the Maintenance mode.

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `startMaintenanceMode()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ---------- | ---------- | ----------- | -------- |
| -6 | SDB_INVALIDARG | Parameter type error. | Check whether the parameter type is correct. |
| -259 | SDB_OUT_OF_BOUND | Required parameters not specified. | Check for missing required parameters. |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v5.8 and above

##EXAMPLES##

Start Maintenance mode in replication group "group1".

```lang-javascript
> var rg = db.getRG("group1")
> rg.startMaintenanceMode({Location: "GuangZhou", MinKeepTime: 100, MaxKeepTime: 1000})
```

Get maintenance mode's detail in group1
```lang-javascript
> db.list(SDB_LIST_GROUPMODES, {GroupID: 1001})
```

[^_^]:
    Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
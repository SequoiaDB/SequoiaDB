##NAME##

startCriticalMode - start Critical mode in the current replication group

##SYNOPSIS##

**rg.startCriticalMode(\<options\>)**

##CATEGORY##

SdbReplicaGroup

##DESCRIPTION##

This function is used to start the Critical mode in a replication group that does not meet the election conditions, and elect the primary node within the specified node range.

##PARAMETERS##

options ( *object, required* )

Specify the Critical mode attributes through the parameter "options":

- NodeName ( *string* ): The effective node in Critical mode.

    The specified node needs to exist in the current replication group.

    Format: `NodeName:"sdbserver:11820"`

- Location ( *string* ): The effective location in Critical mode.

    - The specified location needs to exist in the current replication group.
    - This parameter takes effect only when parameter "NodeName" is not specified.

    Format: `Location: "GuangZhou"`

    > **Note:**  
    >
    > When the catalog replication group starts Critical mode, the effective node must contain the primary node of the current replication group.

- MinKeepTime ( *number* ): Minimum keep time for Critical mode. The value range is (0, 10080], and the unit is minutes.

    Format: `MinKeepTime: 100`

- MaxKeepTime ( *number* ): Maximum keep time for Critical mode. The value range is (0, 10080], and the unit is minutes.

    Format: `MaxKeepTime: 1000`

    > **Note:**
    >
    > For parameters "MinKeepTime" and "MaxKeepTime":
    > - The value of "MinKeepTime" should be less than "MaxKeepTime".
    > - After successfully starting the Critical mode, the replication group will remain in the Critical mode until the time specified by "MinKeepTime" is reached. During the period from "MinKeepTime" to "MaxKeepTime", if most nodes in the replication group are normal, the Critical mode will be automatically released. After the time specified by "MaxKeepTime" expires, the replication group will forcibly cancel the Critical mode.

- Enforced ( *boolean* ): Whether to force to start Critical mode, the default value is "false".

    This parameter is optional, and the values are as follows:

    - true: The replication group will forcibly generate a primary node within the effective node range of the Critical mode. If there is a node with a higher LSN outside the effective node range, the forced execution will cause the data to be rolled back.
    - false: During the start process, if a node with a higher LSN is detected outside the effective node range, the operation will report an error.

    Format: `Enforced: true`

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `startCriticalMode()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ---------- | ---------- | ----------- | -------- |
| -6 | SDB_INVALIDARG | Parameter type error. | Check whether the parameter type is correct. |
| -13 | SDB_TIMEOUT | Start Critical mode timeout. | Check whether there is a node with a higher LSN outside the valid node range. |
| -259 | SDB_OUT_OF_BOUND | Required parameters not specified. | Check for missing required parameters. |
| -334 | SDB_OPERATION_CONFLICT | Parameter range error. | Check whether the primary node of the catalog is within the effective node range of the Critical mode. |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.6.1 and above

##EXAMPLES##

Start Critical mode in replication group "group1".

```lang-javascript
> var rg = db.getRG("group1")
> rg.startCriticalMode({Location: "GuangZhou", MinKeepTime: 100, MaxKeepTime: 1000})
```

Get critical mode's detail in group1
```lang-javascript
> db.list(SDB_LIST_GROUPMODES, {GroupID: 1001})
```

[^_^]:
    Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
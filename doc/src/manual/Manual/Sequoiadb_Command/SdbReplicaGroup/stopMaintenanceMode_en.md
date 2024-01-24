##NAME##

stopMaintenanceMode - stop Maintenance mode in the current replication group

##SYNOPSIS##

**rg.stopMaintenanceMode()**

##CATEGORY##

SdbReplicaGroup

##DESCRIPTION##

This function is used to stop Maintenance mode in the current replication group.

##PARAMETERS##

options ( *object, optional* )

Specify the Maintenance mode attributes through the parameter "options":

- NodeName ( *string* ): The effective node in Maintenance mode.

    The specified node needs to exist in the current replication group.

    Format: `NodeName:"sdbserver:11820"`

- Location ( *string* ): The effective location in Maintenance mode.

    - The specified location needs to exist in the current replication group.
    - This parameter takes effect only when parameter "NodeName" is not specified.

    Format: `Location: "GuangZhou"`

> **Note:**
>
> If the options parameter is missing or the options is empty {}, the command will stop Maintenance mode for all nodes in replica group.

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v5.8 and above

##EXAMPLES##

Stop Maintenance mode for all nodes in replication group "group1".

```lang-javascript
> var rg = db.getRG("group1")
> rg.stopMaintenanceMode()
```

Stop Maintenance mode for the nodes with GuangZhou Location in replication group "group1".

```lang-javascript
> var rg = db.getRG("group1")
> rg.stopMaintenanceMode({Location: GuangZhou})
```

[^_^]:
    Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
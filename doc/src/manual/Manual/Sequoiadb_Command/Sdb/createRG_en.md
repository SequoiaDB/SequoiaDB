##NAME##

createRG - create a replication group

##SYNOPSIS##

**db.createRG(\<name\>)**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to create a replication group. After creation, the system automatically assigns a GroupID to the replication group.

##PARAMETERS##

| Name | Type | Description | Required or not |
| ------ | ------ | ------ | ------ |
| name | string | Replication group name, in the same database object, the replication group name is unique. | required |

> **Note:**
>
> The replication group name cannot be an empty string, and it cannot contain dots(.) or dollar signs($). Its length cannot exceed 127B.

##RETURN VALUE##

When the function executes successfully, it will return an object of type SdbReplicaGroup.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

Create a new replication group named "group1".

```lang-javascript
> db.createRG("group1")
```


[^_^]:
   links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
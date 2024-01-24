##NAME##

removeCoordRG - delete the coordination replication group in the database

##SYNOPSIS##

**db.removeCoordRG()**

##CATEGORY##

Sdb

##DESCRIPTION##

The function is used to delete the coordination replication group in the database. In principle, this operation will delete all coordination nodes of the replication group. However, if the coordination node connected to the db object is deleted first during the process of deleting nodes, some coordination nodes may be left. At this point, users need to use [Oma.removeCoord()][removeCoord] to delete remaining coordination node.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERROR##

When the exception happens，use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

Delete the coordination replication group.

```lang-javascript
> db.removeCoordRG()
```



[^_^]:
    links
[removeCoord]:manual/Manual/Sequoiadb_Command/Oma/removeCoord.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
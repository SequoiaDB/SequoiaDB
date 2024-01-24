##NAME##

removeCataRG - delete the catalog replication group in the database

##SYNOPSIS##

**db.removeCataRG()**

##CATEGORY##

Sdb

##DESCRIPTION##

The function is used to delete the catalog replication group in the database. In principle, this operation will delete all catalog nodes of the replication group. Therefore, the data node and coordination node information cannot exist in the targe replication group.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens，use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v1.10 and above

##EXAMPLES##

Delete the catalog replication group.

```lang-javascript
> db.removeCataRG()
```


[^_^]:
   links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md

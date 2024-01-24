##NAME##

renameCL - Rename collection.

##SYNOPSIS##

**db.collectionspace.renameCL( \<oldname\>, \<newname\> )**

##CATEGORY##

SdbCS

##DESCRIPTION##

Rename collection.

##PARAMETERS##

* `oldname` ( *String*， *Required* )

	The name of the collection. 

* `newname` ( *String*， *Required* )

	New name of the collection. 

**Note:**

1. The write operations of relevant data node will be blocked until rename operation complete.
2. Do not allow to connect data node to rename collection.

##RETURN VALUE##

On success, return void.

On error, exception will be thrown.

##ERRORS##

the exceptions of `renameCS()` are as below:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -23 | SDB_DMS_NOTEXIST | The cl corresponding to "oldname" does not exist. | Rename an existing collection. |
| -22 | SDB_DMS_EXIST | The cl corresponding to "newname" already exists. | Set newname to a name that does not exist in the database. |
| -67 | SDB_BACKUP_HAS_ALREADY_START | Backup has already been started. | Do rename operation after backup complete. |
| -148 | SDB_DMS_STATE_NOT_COMPATIBLE | Other rename operation has already been started. | Do rename operation after other rename complete. |
| -149 | SDB_REBUILD_HAS_ALREADY_START | Database rebuild has already been started. | Do rename operation after rebuild complete. |

When error happen, use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md)
to get the error message or use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md)
to get the error code. See [troubleshooting](manual/FAQ/faq_sdb.md) for
more detail.

##HISTORY##

* since v3.0.1

##EXAMPLES##

1. Rename cl sample.employee to sample.employee_new.

	```lang-javascript
	> db = new Sdb( "localhost", 11810 )
	> db.sample.renameCL( "employee", "employee_new" )
	```
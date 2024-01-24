##NAME##

renameCS - Rename collection space.

##SYNOPSIS##

**db.renameCS( \<oldname\>, \<newname\> )**

##CATEGORY##

Sdb

##DESCRIPTION##

Rename collection space.

##PARAMETERS##

* `oldname` ( *String*， *Required* )

	The name of the collection space. 

* `newname` ( *String*， *Required* )

	New name of the collection space. 

**Note:**

1. The write operations of relevant data node will be blocked until rename operation complete.
2. Do not allow to connect data node to rename collection spaces.

##RETURN VALUE##

On success, return void.

On error, exception will be thrown.

##ERRORS##

the exceptions of `renameCS()` are as below:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -34 | SDB_DMS_CS_NOTEXIST | The cs corresponding to "oldname" does not exist. | Rename an existing collection space. |
| -33 | SDB_DMS_CS_EXIST | The cs corresponding to "newname" already exists. | Set newname to a name that does not exist in the database. |
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

1. Rename cs sample to sample_new.

	```lang-javascript
	> db = new Sdb( "localhost", 11810 )
	> db.renameCS( "sample", "sample_new" )
	```
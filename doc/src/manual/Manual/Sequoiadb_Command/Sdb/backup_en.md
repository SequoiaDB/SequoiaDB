 
##NAME##

backup - backup database

##SYNOPSIS##

**db.backup([options])**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to backup the entire database or specify the replication group.

##PARAMETERS##

options ( *object, optional* )

The name, path, description and other attributes of the backup can be set through the patameter "options":

- GroupID ( *array* ): The ID of the replication group to be backed up, and the default is all replication groups.

    Format: `GroupID:1000` or `GroupID:[1000, 1001]`

- GroupName ( *string* ): The name of the replication group to be backed up, and the default is all replication groups.

    Format: `GroupName: "data1"` or `GroupName: ["data1", "data2"]`

- Name ( *string* ): Backup name, and the default is the backup name in "YYYY-MM-DD-HH:mm:SS" time format.

    Format: `Name: "backup-2014-1-1"`

- Path ( *string* ): Backup path, and the default is the backup path specified by node configuration item parameter [bkuppath][path].

    This parameter supports wildcard %g/%G(indentification group name), %h/%H(indentification host name) and %s/%S(indentification service name). When coordination node perform backup and specify this parameter, users need to use wildcards to avoid all nodes operating under the same path, resulting in unknown IO errors.

    Format: `Path: "/opt/sequoiadb/backup/%g"`

- IsSubDir ( *boolean* ): Whether the path configured by the parameter "Path" is a subdirectory of the backup path specified in the configuration parameter, and the default is false.

    If the parameter value is true, the real backup directory is: "the backup derectory specified in the configuration parameter/the directory specified by parameter 'Path'".

    Format: `IsSubDir: false`

- Prefix ( *string* ):  Backup prefix name, and the default is null.

    This parameter supports wildcard(%g, %G, %h, %H, %s and %S).

    Format: `Prefix: "%g_bk_"`

- EnableDateDir ( *boolean* ): Whether to enable the date subdirectory function, the default is false.

    If the parameter value is true, a subdirectory named “YYYY-MM-DD” will be automatically created based on the current date.

    Format: `EnableDateDir: false`

- Description ( *string* ): Backup description.

    Format: `Description: "First backup"`

- EnsureInc ( *boolean* ): Whether to enable incremental backup, the default  is false.

    Format: `EnsureInc: false`

- OverWrite ( *boolean* ): Whether to overwrite a backup with the same name, and the default is false.

    Format: `OverWrite: false`

- MaxDataFileSize ( *number* ): Specify the maximum size of backup data file. The default value is 102400, the unit is MB, and the value range is [32, 8388608], i.e., 32MB~8TB.

    Backup data files that exceed the specified size will be split into several files that meet the specifications.

    Format: `MaxDataFileSize: 64`

- Compressed ( *boolean* ): Whether to enable data compression, and the default is true.

    Format: `Compressed: true`

- CompressionType ( *string* ): Compression format type, the value includes "lz4", "snappy" and "zlib", and the default is "snappy".

    Format: `CompressionType: "zlib" `

- BackupLog ( *boolean* ): Whether all logs need to be backed up when full backup, and the default is false.

    Format: `BackupLog: false`

> **Note:**
>
> Added Compressed, CompressionType and BackupLog parameters for v2.8.2 and above.

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.
 
##ERRORS##

The common exceptions of `backup()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ---------- | ---------- | ----------- | -------- |
| -240   | SDB_BAR_BACKUP_EXIST    | A backup with the same name already exists. | Delete the backup first or set parameter "OverWrite" to true.  |
| -241   | SDB_BAR_BACKUP_NOTEXIST | The full backup corresponding to the incremental backup does not exist. | Perform a full backup first. |
| -70    | SDB_BAR_DAMAGED_BK_FILE | The backup file is corrupted. | - |
| -57    | SDB_DPS_LOG_NOT_IN_BUF  | The start log of the incremental backup no longer exists. | Perform an incremental backup after re-executing a full backup. |
| -98    | SDB_DPS_CORRUPTED_LOG   | The hash check of the same log is inconsistent, and the log has changed. | Perform an incremental backup after re-executing a full backup. |

When the exception happens，use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v1.2 and above

##EXAMPLES##

- Perform a full backup of the replication group "group1".

    ```lang-javascript
    > db.backup({Name: "backupName", Description: "backup group1", GroupName: "group1"})
    ```

- Perform a full backup on the secondary node.

    ```lang-javascript
    > db.setSessionAttr({PreferredInstance: "S", PreferredConstraint: "secondaryonly"})
    > db.backup({Name: "backupName", Description: "backup on secondary"})
    ```

[^_^]:
    links
[path]:manual/Distributed_Engine/Maintainance/Database_Configuration/configuration_parameters.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md

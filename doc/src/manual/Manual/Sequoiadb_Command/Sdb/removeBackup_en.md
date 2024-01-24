##NAME##

removeBackup - remove a database backup

##SYNOPSIS##

**db.removeBackup([options])**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to remove a database backup.

##PARAMETERS##

options ( *object, optional* )

Parameters such as backup name, replication group, and backup path can be set through the options parameter:

- GroupID ( *number/array* ): The ID of the backup replication group, the default is all replication groups.

    Format: `GroupID: 1000` or `GroupID:[1000, 1001]`

- GroupName ( *string/array* ): The name of the backup replication group, the default is all replication groups.

    Format: `GroupName: "data1"` or `GroupName: ["data1", "data2"]`

- Name ( *string* ): Backup name, all backups are deleted by default.

    Format: `Name: "backup-2014-1-1"`

- Path ( *string* ): Backup path, the default is the backup  path specified by the configuration parameter.

    The path supports wildcards(%g/%G: group name, %h/%H:  host name, %s/%S: service name).

    When executing commands on the coordination node and using this parameter, wildcards are required to avoid all nodes operating under the same path and causing unknown IO errors.

    Format: `Path: "/opt/sequoiadb/backup/%g"`

- IsSubDir ( *boolean* ): Whether the path configured by the above path parameter is a subdirectory of the backup path specified by the configuration parameter, and the default is false.

    If this parameter is true, the real backup directory is: `backup directory specified in the configuration parameter/path directory`.

    Format: `IsSubDir: false`

- Prefixr ( *string* ): Backup prefix name. It supports wildcards(%g,%G,%h,%H,%s,%S), and the default is empty

    Format: `Prefix: "%g_bk_"`

- ID ( *number* ): Backup ID, the default is -1, which indentifies all backups with this name.

    Format: `ID: -1`

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

Add ID parameter in v2.8.2 and above.

##EXAMPLES##

Remove the backup information named "backup-2014-1-1" in the database.

```lang-javascript
> db.removeBackup({Name: "backup-2014-1-1"})
```

[^_^]:
   links
[location]:manual/Manual/Sequoiadb_Command/location.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
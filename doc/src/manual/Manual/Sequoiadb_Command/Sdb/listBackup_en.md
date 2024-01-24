##NAME##

listBackup - list the backup information

##SYNOPSIS##

**db.listBackup([options], [cond], [sel], [sort])**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to list the backup information of the current database.

##PARAMETERS##

- options ( *object, optional* )

    The options parameter can be used to filter the backup information to be enumerated:

    - GroupID ( *number/array* ): ID of the replication group, and the default is all replication groups.

        Format: `GroupName: "data1"` or `GroupName: ["data1", "data2"]`

    - GroupName ( *string* ): Name of the replication group, and the default is all replication groups.

        Format: `GroupName: "group1"`

    - Name ( *string* ): Backup name, and the default is all backups.

        Format: `Name: "backup-2014-1-1"`

    - Path ( *string* ): Backup path, and the default is the backup path specified by the configuration parameter.

        This parameter supports wildcard %g/%G(represents group name), %h/%H(represents host name) and %s/%S(represents service name).

        Format: `Path: "/opt/sequoiadb/backup/%g"`

    - IsSubDir ( *boolean* ): Whether the path configured by the parameter "Path" is a subdirectory of the backup path specified in the configuration parameter, and the default is false.

        Format: `IsSubDir: false`

    - Prefix ( *string* ): Backup prefix name, and the default is empty.

        This parameter supports wildcards(%g, %G, %h, %H, %s and %S).

        Format: `Prefix: "%g_bk_"`

    - Detail ( *boolean* ): Whether to display the detailed information of the backup, and the default is false.

        Format: `Detail: true`

- cond ( *object, optional* )

    Specify the filter criteria for the returned records.

- sel ( *object, optional* )

    Specify the output field for the the returned record.

- sort ( *object, optional* )

    Sort the returned records according to the selected field. The values are as follows:

    - 1: Ascending
    - -1: Descending

##RETURN VALUE##

When the function executes successfully, it will return an object of type SdbCursor. Users can get a list of backup details through this object. For field descriptions, refer to [$LIST_BACKUP][LIST_BACKUP].

##ERRORS##

When the exception happens，use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

- List the backup information of the backup path specified by the database configuration parameter.

    ```lang-javascript
    > db.listBackup()
    {
      "Version": 2,
      "Name": "test",
      "ID": 0,
      "NodeName": "vmsvr2-suse-x64-1:20000",
      "GroupName": "db1",
      "EnsureInc": false,
      "BeginLSNOffset": 195652020,
      "EndLSNOffset": 195652068,
      "TransLSNOffset": -1,
      "StartTime": "2017-06-20-13:02:22",
      "LastLSN": 195652020,
      "LastLSNCode": 1845751176,
      "HasError": false
    }
    ```

- List backup information of other paths.

    Backup data node to other paths.

    ```lang-javascript
    > var datadb = new Sdb("sdbserver", 20000)
    > datadb.backup({Path: "/tmp/sequoiadb_backup/20000"})
    ```

    Connect to the coordination node to list backup information

    ```lang-javascript
    > var db = new Sdb("sdbserver", 11810) 
    > db.listBackup({Path: "/tmp/sequoiadb_backup/20000"})
    {
      "Version": 2,
      "Name": "2017-10-26-10:14:11",
      "ID": 0,
      "NodeName": "sdbserver:20000",
      "GroupName": "db1",
      "EnsureInc": false,
      "BeginLSNOffset": -1,
      "EndLSNOffset": 375546828,
      "TransLSNOffset": -1,
      "StartTime": "2017-10-26-10:14:11",
      "LastLSN": -1,
      "LastLSNCode": 0,
      "HasError": false
    }
    ```

    > **Note:**
    >
    > After modifying the backup path, users can execute listBackup() to specify the parameter "Path", otherwise the backup information cannot be found under the default path.


[^_^]:
     本文使用的所有引用及链接
[LIST_BACKUP]:manual/Manual/SQL_Grammar/Monitoring/LIST_BACKUP.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
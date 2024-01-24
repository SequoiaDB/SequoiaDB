
##NAME##

getFile - Create a file object remotely.

##SYNOPSIS##

***remoteObj.getFile( \<filepath\>, \[permission\], \[mode\] )***

##CATEGORY##

Remote

##DESCRIPTION##

Create a file object remotely.

##PARAMETERS##

| Name       | Type     | Default                               | Description        | Required or not |
| ---------- | -------- | ------------------------------------- | ------------------ | -------- |
| filepath   | string   | ---                                   | file path                      | not      |
| permission | int      | 0700                                  | set permissions to open files  | not      |
| mode       | int      | SDB_FILE_READWRITE \| SDB_FILE_CREATE | set the way to open files      | not      |

>Note：

>If you want to execute the global method of the remote File class, you can leave the parameters blank. For more detial of the global method, you can execute the 'File.help()' in the sdb shell to view.

The optional values of the 'mode' parameter are as follows：

| Optional values       | Description                                                       |
| --------------------- | ----------------------------------------------------------------- |
| SDB_FILE_CREATEONLY   | create a new file only                                            |
| SDB_FILE_REPLACE      | overwrite the contents of the original file and write new content |
| SDB_FILE_CREATE       | create a new file and open it                                     |
| SDB_FILE_READONLY     | open the file in read-only mode                                   |
| SDB_FILE_WRITEONLY    | open the file in write-only mode                                  |
| SDB_FILE_READWRITE    | open the file in read-write mode                                  |
| SDB_FILE_SHAREREAD    | open the file in shared read  mode                                |
| SDB_FILE_SHAREWRITE   | open the file in shared write mode                                |

>Note：

>These flags can be combined with bitwise or operator( | ).

##RETURN VALUE##

On success, if open a file, return a file descriptor. If create a new file, return void.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Create a remote object.

```lang-javascript
> var remoteObj = new Remote( "192.168.20.71", 11790 )
```

* Create a file object remotely.

```lang-javascript
> var file = remoteObj.getFile( "/opt/sequoiadb/file", 0777, SDB_FILE_READWRITE | SDB_FILE_CREATE )
```

* Read the contents of the file.( For more detial, please  reference to [read](manual/Manual/Sequoiadb_command/File/read.md) ).

```lang-javascript
> file.read()
SquoiaDB
```
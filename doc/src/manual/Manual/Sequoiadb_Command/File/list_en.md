
##NAME##

list - List directory contents.

##SYNOPSIS##

***File.list( \<options\>, \[filter\] )***

##CATEGORY##

File

##DESCRIPTION##

List directory contents.

##PARAMETERS##

| Name    | Type     | Default                        | Description         | Required or not |
| ------- | -------- | ------------------------------ | ------------------- | --------------- |
| options | JSON     | ---                            | optional parameter  | yes             |
| filter  | JSON     | Default to display all content | filtered conditions | not             |

The detail description of 'options' parameter is as follow:

| Attributes | Type    | Description                    | Required or not  |
| ---------- | ------- |------------------------------- | ---------------- |
| detail     | boolean | whether to display the details | not              |
| pathname   | string  | filepath                       | not              |

The optional parameter Filter supports the AND, the OR, the NOT and exact matching of some fields in the result, and the result set is filtered.

##RETURN VALUE##

On success, return file information in the specified directory.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* List directory contents

```lang-javascript
> File.list( { detail: true, pathname: "/opt/sequoiadb" } )
{
  "name": "file1.txt",
  "size": "20480",
  "mode": "drwxr-xr-x",
  "user": "root",
  "group": "root",
  "lasttime": "6月 11 11:58"
}
{
  "name": "file2.txt",
  "size": "20480",
  "mode": "drwxr-xr-x",
  "user": "root",
  "group": "root",
  "lasttime": "6月 12 12:58"
}
{
  "name": "file3.txt",
  "size": "20480",
  "mode": "drwxr-xr-x",
  "user": "root",
  "group": "root",
  "lasttime": "6月 13 13:58"
}
```

* List directory contents and filter the result set

```lang-javascript
> File.list( { detail: true, pathname: "/opt/sequoiadb" }, { $and: [ { name: "file1" }, { size: "20480" } ] } )
{
  "name": "file1.txt",
  "size": "20480",
  "mode": "drwxr-xr-x",
  "user": "root",
  "group": "root",
  "lasttime": "6月 13 13:58"
}
```
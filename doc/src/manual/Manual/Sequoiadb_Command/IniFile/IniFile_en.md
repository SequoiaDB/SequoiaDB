
##NAME##

IniFile - Open a INI configuration file.

##SYNOPSIS##

***var ini = new IniFile( \<filename\>, \[flags\] )***

##CATEGORY##

IniFile

##DESCRIPTION##

Open a INI configuration file.

##PARAMETERS##

| Name       | Type     | Default                   | Description                   | Required or not |
| ---------- | -------- | ------------------------- | ----------------------------- | --------------- |
| filename   | string   | ---                       | file path                     | yes             |
| flags      | int      | SDB_INIFILE_FLAGS_DEFAULT | flags for parse ini configure | not             |

The optional values of the 'flags' parameter are as follows：

| Optional values              | Description                                     |
| ---------------------------- | ----------------------------------------------- |
| SDB_INIFILE_NOTCASE          | not case sensitive                              |
| SDB_INIFILE_SEMICOLON        | support semicolon ( ; ) annotation              |
| SDB_INIFILE_HASHMARK         | support the hash mark ( # ) annotation          |
| SDB_INIFILE_ESCAPE           | support for escape characters, for example: \\n |
| SDB_INIFILE_DOUBLE_QUOMARK   | support for value with double quotes ( " )      |
| SDB_INIFILE_SINGLE_QUOMARK   | support for value with single quotes ( ' )      |
| SDB_INIFILE_EQUALSIGN        | support for the equal sign ( = ) key separator  |
| SDB_INIFILE_COLON            | support for colon ( : ) key separators          |
| SDB_INIFILE_UNICODE          | support for Unicode string                      |
| SDB_INIFILE_STRICTMODE       | turn on strict mode, do not allow duplicate section names and key names |
| SDB_INIFILE_FLAGS_DEFAULT    | the default flags, equivalent to SDB_INIFILE_SEMICOLON \| SDB_INIFILE_EQUALSIGN \| SDB_INIFILE_STRICTMODE |

> Note：  
> These flags can be combined with bitwise or operator( | ).

##RETURN VALUE##

On success, return a IniFile object.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Create a new file and open it in read-write mode.

```lang-javascript
> var ini = new IniFile( "/opt/sequoiadb/file.ini", SDB_INIFILE_FLAGS_DEFAULT )
```
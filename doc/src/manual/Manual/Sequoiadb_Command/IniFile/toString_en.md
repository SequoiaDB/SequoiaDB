
##NAME##

toString - Convert to text.

##SYNOPSIS##

***IniFile.toString()***

##CATEGORY##

IniFile

##DESCRIPTION##

Convert to text.

##PARAMETERS##

NULL

##RETURN VALUE##

On success, return string.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Open an INI file.

```lang-javascript
> var ini = new IniFile( "/opt/sequoiadb/file.ini", SDB_INIFILE_FLAGS_DEFAULT )
```

* Convert to text.

```lang-javascript
> ini.toString()
[info]
name=Alan
age=23
```
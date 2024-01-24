
##NAME##

toObj - Convert to object.

##SYNOPSIS##

***IniFile.toObj()***

##CATEGORY##

IniFile

##DESCRIPTION##

Convert to object.

##PARAMETERS##

NULL

##RETURN VALUE##

On success, return object.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Open an INI file.

```lang-javascript
> var ini = new IniFile( "/opt/sequoiadb/file.ini", SDB_INIFILE_FLAGS_DEFAULT )
```

* Convert to object.

```lang-javascript
> ini.toObj()
{
  "info.name": "Alan",
  "info.age": 23
}
```
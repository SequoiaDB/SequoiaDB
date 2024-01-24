
##NAME##

delComment - Delete the comment for the specified section.

##SYNOPSIS##

***IniFile.delSectionComment( \<section\> )***

##CATEGORY##

IniFile

##DESCRIPTION##

Delete the comment for the specified section.

##PARAMETERS##

| Name     | Type     | Default | Description                            | Required or not |
| -------- | -------- | --------| -------------------------------------- | --------------- |
| section  | string   | ---     | section name                           | yes             |

##RETURN VALUE##

On success, return void.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Open an INI file.

```lang-javascript
> var ini = new IniFile( "/opt/sequoiadb/file.ini", SDB_INIFILE_FLAGS_DEFAULT )
```

* Delete the comment for the specified section.

```lang-javascript
> ini.delComment( "info" )
```
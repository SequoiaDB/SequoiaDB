
##NAME##

setSectionComment - Set a comment to the specified section.

##SYNOPSIS##

***IniFile.setSectionComment( \<section\> \<comment\> )***

##CATEGORY##

IniFile

##DESCRIPTION##

Set a comment to the specified section.

##PARAMETERS##

| Name     | Type     | Default | Description   | Required or not |
| -------- | -------- | --------| ------------- | --------------- |
| section  | string   | ---     | section name  | yes             |
| comment  | string   | ---     | comment       | yes             |

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

* Set a comment to the specified section.

  ```lang-javascript
  > ini.setSectionComment( "info", "personal information" )
  ```
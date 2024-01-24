
##NAME##

disableItem - Comment the specified item.

##SYNOPSIS##

***IniFile.disableItem( \<section\>, \<key\> )***

***IniFile.disableItem( \<key\> )***

##CATEGORY##

IniFile

##DESCRIPTION##

Comment the specified item.

##PARAMETERS##

| Name     | Type     | Default | Description                            | Required or not |
| -------- | -------- | --------| -------------------------------------- | --------------- |
| section  | string   | ---     | section name                           | yes             |
| key      | string   | ---     | item key name                          | yes             |

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

* Comment the specified item.

```lang-javascript
> ini.disableItem( "info", "name" )
```
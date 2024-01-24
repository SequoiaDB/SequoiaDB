##NAME##

dropDataSource - Drop data source

##SYNOPSIS##

**db.dropDataSource(\< Name \>)**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to drop the specified data source. Users need to ensure that no database object are using the datasource when dropping it. That is, the datasource is not associated with any collection space or collection. 

##PARAMETERS##

Name ( *string，required* )

Name of the data source

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

`dropDataSource()` The common exceptions of functions are as follows:

| error code | Error type | Possible reason | Solution |
| ------ | -------- | -------------- | -------- |
| -370   | SDB_CAT_DATASOURCE_NOTEXIST | Data source does not exist | Check whether the specified data source exists. |
| -371   | SDB_CAT_DATASOURCE_INUSE   | Data source has been used | Delete the associated collection space or collection in the data source when deleting a data source. |

When the exception happens, use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get the error message or use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the error code. For more details, refer to [Troubleshooting](troubleshooting/general/general_guide.md).

##VERSION##

v3.2.8 and above

##EXAMPLES##

Drop the data source named "datasource"

```lang-javascript
> db.dropDataSource("datasource")
```


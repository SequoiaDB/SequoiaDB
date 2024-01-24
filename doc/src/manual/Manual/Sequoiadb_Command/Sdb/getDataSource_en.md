##NAME##

getDataSource - Get a reference to the data source

##SYNOPSIS##

**db.getDataSource(\<name\>)**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to get a reference of a data source, which can modify the metadata information of the data source.

##PARAMETERS##

name ( *string*, *required* )

Name of the data source

##RETURN VALUE##

When the function executes successfully, it will return a DataSource object.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

`getDataSource()` The common exceptions of functions are as follows:

| error code | Error type | Possible reason | Solution |
| ------ | -------- | -------------- | -------- |
| -370   | SDB_CAT_DATASOURCE_NOTEXIST | Data source does not exist | Check if the specified data source exists |

When the exception happens, use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get the error message or use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the error code. For more details, refer to [Troubleshooting](troubleshooting/general/general_guide.md).

##VERSION##

v3.2.8 and above

##EXAMPLES##

Get a reference of data source

```lang-javascript
> var ds = db.getDataSource("datasource")
```

Modify the data source access as "WRITE"

```lang-javascript
> ds.alter({AccessMode:"WRITE"})
```
##NAME##

listDataSources - View the metadata information of the data source

##SYNOPSIS##

**db.listDataSources([cond], [sel], [sort])**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to view metadata information such as the service address, access authority, data source version number, and type of the data source.

##PARAMETERS##

| Name   | Type    | Description   													| Required or Not
|----------|-------------|----------------------------------------------------------|----------|
| cond     | object | Match condition and only return records that match cond, when it is null.| Not 	   |
| sel      | object | Select the returned field name.When it is null, all field names are returned | Not 	   |
| sort     | object | Sort the returned records according to the selected field, 1 is ascending order, -1 is descending order | Not 	   |

##RETURN VALUE##

When the function executes successfully, it will return a detailed list of collections through the SdbCursor.Users can refer to [SYSCAT.SYSDATASOURCES collection](infrastructure/catalog_node/SYSDATASOURCES.md) to get the returned field information.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get the error message or use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the error code. For more details, refer to [Troubleshooting](troubleshooting/general/general_guide.md).

##VERSION##

v3.2.8 and above

##EXAMPLES##

View the metadata information of the data source

```lang-javascript
> db.listDataSources()
```

The output results are as follows:

```lang-json
{
  "_id": {
    "$oid": "5ffc365c72e60c4d9be30c50"
  },
  "ID": 2,
  "Name": "datasource",
  "Type": "SequoiaDB",
  "Version": 0,
  "DSVersion": "3.4.1",
  "Address": "sdbserver:11810",
  "User": "sdbadmin",
  "Password": "d41d8cd98f00b204e9800998ecf8427e",
  "ErrorControlLevel": "low",
  "AccessMode": 1,
  "AccessModeDesc": "READ",
  "ErrorFilterMask": 0
  "ErrorFilterMaskDesc": "NONE"
}
```

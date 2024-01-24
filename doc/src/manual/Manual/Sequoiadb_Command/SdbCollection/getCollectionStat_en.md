##NAME##

getCollectionStat - get statistics of the collection

##SYNOPSIS##

**db.collectionspace.collection.getCollectionStat()**

##CATEGORY##

SdbCollection

##DESCRIPTION##

This function is used to get statistics of the current collection.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, it will return an object of type BSONObj. Users can get the statistics of the collection through this object. For fields are described as follows:

| Name            | Type    | Description                                                                                       |
| --------------- | ------- | ------------------------------------------------------------------------------------------------- |
| Collection      | string  | Collection name.                                           |
| InternalV       | uint32  | Version of collection statistics.                          |
| StatTimestamp   | string  | Statistical time of collection statistics.                 |
| IsDefault       | boolean | Whether collection statistics are the default values.     |
| IsExpired       | boolean | Whether the collection statistics are out of date, and the default value is "false".            |
| AvgNumFields    | uint64  | The average number of fields in the records in the collection, and the default value is 10.           |
| SampleRecords   | uint64  | The number of records to sample in the collection, and the default value is 200.                      |
| TotalRecords    | uint64  | The total number of records in the collection, and the default value is 200.                          |
| TotalDataPages  | uint64  | The total number of data pages in the collection, and the default value is 1.                         |
| TotalDataSize   | uint64  | The total size of the data in the collection, the default value is 80000, and the unit is byte.       |

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common expretions of `getCollectionStat()` function are as follows:

| Error Code | Error Type             | Description                   | Solution                 |
| ---------- | ---------------------- | ------------------------------| ------------------------ |
| -377       | SDB_MAINCL_NOIDX_NOSUB |The main collection being operated does not have a sub-collection mounted. | Use [attachCL()][attachCL] to mount any sub-collection on that main collection. |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.6.1 and above

##EXAMPLES##

Get statistics of collection "sample.employee".

```lang-javascript
> db.sample.employee.getCollectionStat()
```

The output is as follows:

```lang-json
{
  "Collection": "sample.employee",
  "InternalV": 1,
  "StatTimestamp": "2022-11-07-20.29.43.739000",
  "IsDefault": false,
  "IsExpired": false,
  "AvgNumFields": 10,
  "SampleRecords": 1000,
  "TotalRecords": 1000,
  "TotalDataPages": 22,
  "TotalDataSize": 30000
}
```

>**Note**
>
> When the value of the field "IsDefault" is "true", it means that the statistical information of the corresponding collection has not been queried. Users need to manually execute [analyze()][analyze] to update the statistical information of the collection, and then perform the operation of obtaining statistical information.

[^_^]:
     Links
[analyze]:manual/Manual/Sequoiadb_Command/Sdb/analyze.md
[attachCL]:manual/Manual/Sequoiadb_Command/SdbCollection/attachCL.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[faq]:manual/FAQ/faq_sdb.md

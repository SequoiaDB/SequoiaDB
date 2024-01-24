##NAME##

snapshotIndexes - get the index information of the collection

##SYNOPSIS##

**db.collectionspace.collection.snapshotIndexes([cond], [sel], [sort])**

##CATEGORY##

SdbCollection

##DESCRIPTION##

This function is used to obtain all index information of the specified collection. Users execute the function through the coordination node, and collect and aggregate index information from all data nodes related to the collection; execute the function through the data node, and obtain the index information from the data node.

##PARAMETERS##

- cond ( *object, optional* )

    Set matching condictions and [command position parameter][parameter], and select all records by default.

- sel ( *object, optional* )

    Select the returned field name. The default value is null, and return all field names.

- sort ( *object, optional* )

    Sort the returned records by the selected field. The values are as follows:
        
    - 1: ascending 
    - -1: descending

> **Note:**
>
> - sel parameter is a json structure,like:{Field name:Field value}，The field value is generally specified as an empty string.The field name specified in sel exists in the record,setting the field value does not take effect. Return the field name and field value specified in the sel otherwise.
> - The field value type in the record is an array.Users can specify the field name in sel,and use "." operator with double marks ("") to refer to the array elements.

##RETURN VALUE##

When the function executes successfully, it will return an object of type SdbCursor. Users can get the index details through this object. For field descriptions, refer to [SYSINDEXES][SYSINDEXES].

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.6 and above

##EXAMPLES##

Get index information of collection sample.employee.

```lang-javascript
> db.sample.employee.snapshotIndexes()
{
  "IndexDef": {
    "name": "$id",
    "_id": {
      "$oid": "6098e71a820799d22f1f2164"
    },
    "UniqueID": 4037269258240,
    "key": {
      "_id": 1
    },
    "v": 0,
    "unique": true,
    "dropDups": false,
    "enforced": true,
    "NotNull": false,
    "NotArray": true,
    "Global": false,
    "Standalone": false,
  },
  "IndexFlag": "Normal",
  "Type": "Positive",
  "ExtDataName": null,
  "Nodes": [
    {
      "NodeName": "sdbserver1:11820",
      "GroupName": "group1"
    },
    {
      "NodeName": "sdbserver1:11830",
      "GroupName": "group2"
    }
  ]
}
```

[^_^]:
    Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[parameter]:manual/Manual/Sequoiadb_Command/location.md
[SYSINDEXES]:manual/Manual/Catalog_Table/SYSINDEXES.md
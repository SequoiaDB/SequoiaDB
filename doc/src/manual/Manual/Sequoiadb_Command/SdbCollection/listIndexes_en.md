##NAME##

listIndexes - list the index information in the collection

##SYNOPSIS##

**db.collectionspace.collection.listIndexes\(\)**

##CATEGORY##

SdbCollection

##DESCRIPTION##

This function is used to list the information of all [indexes][index] in the specified collection. When the users executes the function through the coordination node, the index information will be obtained from the catalog node. If the function is executed through the data node, the index information will be obtained from the data node.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, it will return an object of type SdbCursor. Users can get a list of index details through this object, the field descriptions are as follows:

| Name    | Type  | Description   | 
| ------    | --------  | ------ |
| name      | string    | Index name |
| key       | json    | Index key, the value is as follows:<br>1: Ascending by field<br>-1: Descending order by field<br>"text": [Full-text index][text_index]        |
| v         | int32     | Index version number                                   |
| unique    | boolean   | Is the index unique, the value is as follows:<br> "true": Unique index, no duplicate values in the collection are allowed.<br> "false": Ordinary index, allowing duplicate values in the collection.                                   | 
| enforced  | boolean   | Whether the index is mandatory to be unique, the value is as follows:<br>"false": Not mandatory.<br>"true": Mandatory unique, which means that more than one empty index key is not allowed.      |
| NotNull   | boolean   | Whether any field of the index is allowed to be "null" or non-existent, the value is as follows: <br> "true": Not allowed to be "null" or non-existent. <br> "false": Allow "null" or not exist.    |
| IndexFlag | string    | Index current state, the value is as follows: <br> "Normal": Normal <br> "Creating": Creating <br> "Dropping": Dropping <br> "Truncating": Truncating <br> "Invalid": Invalid                                                       |
| Type      | string    | Index type, the value is as follows:<br> "Positive": Positive index <br> "Reverse": Reverse index <br> "Text": Full-text index                                     |
| NotArray| boolean   | Whether any field of the index is allowed to be an array, the value is as follows:<br> "true": Not allowed to be an array. <br> "false": Allowed as an array.    |
|Standalone| boolean    | Whether it is an independent index. |
| dropDups  | boolean   | Not open                                  |

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.4 and above

##EXAMPLES##

List the information of all indexes in the collection "sample.employee".

```lang-javascript
> db.sample.employee.listIndexes()
{
  "_id": {
    "$oid": "6098e71a820799d22f1f2165"
  },
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
    "Standalone": false
  },
  "IndexFlag": "Normal",
  "Type": "Positive"
}
```

[^_^]:
     Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[createIndex]:manual/Manual/Sequoiadb_Command/SdbCollection/createIndex.md
[SDB_SNAP_INDEXSTATS]:manual/Manual/Snapshot/SDB_SNAP_INDEXSTATS.md
[index]:manual/Distributed_Engine/Architecture/Data_Model/index.md
[indexDef]:manual/Manual/Sequoiadb_Command/SdbCollection/createIndex.md
[enforced]:manual/Manual/Sequoiadb_Command/SdbCollection/createIndex.md
[text_index]:manual/Distributed_Engine/Architecture/Data_Model/text_index.md

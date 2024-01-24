##NAME##

getIndex - get the specified index

##SYNOPSIS##

**db.collectionspace.collection.getIndex(\<name\>)**

##CATEGORY##

SdbCollection

##DESCRIPTION##

This function is used to get the specified index from current collection.

##PARAMETERS##

name ( *string, required* )

Specify the index name to be obtained.

> **Note**
>
> * Index name should not contain null string, "." or "$". The length of it should not be greater than 127B.

##RETURN VALUE##

When the function executes successfully, it will return an object of type BSONObj. Users can get the index details through this object. For field descriptions, refer to [SYSINDEXES][SYSINDEXES].

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

Frequent exceptions of `getIndex()`：

|Error Code|Error Name|Causes|Solution|
|----------|----------|------|--------|
|-47       |SDB_IXM_NOTEXIST |Index doesn't exist | Check if the index exists|

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the error code. For more details, refer to [Troubleshooting][faq].

##VERSION##

v1.10 and above

##EXAMPLES##

Get the index named "$id" from the sample.employee collection.

```lang-javascript
> db.sample.employee.getIndex("$id")
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
    links
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[faq]:manual/FAQ/faq_sdb.md
[SYSINDEXES]:manual/Manual/Catalog_Table/SYSINDEXES.md
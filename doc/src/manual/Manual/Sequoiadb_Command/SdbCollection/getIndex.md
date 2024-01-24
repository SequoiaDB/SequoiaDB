##名称##

getIndex - 获取指定索引

##语法##

**db.collectionspace.collection.getIndex(\<name\>)**

##类别##

SdbCollection

##描述##

该函数用于获取当前集合中指定的索引信息。

##参数##

name（ *string，必填* ）

指定需要获取的索引名

> **Note:**
>
> 索引名不能是空串，含点（.）或者美元符号（$），且长度不超过127B。

##返回值##

函数执行成功时，将返回一个 BSONObj 类型的对象。通过该对象获取集合详细信息列表，字段说明可参考 [SYSINDEXES 集合][SYSINDEXES]。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`getIndex()` 函数常见异常如下：

|错误码|错误名|可能发生的原因|解决办法|
|------|------|--------------|--------|
| -47  | SDB_IXM_NOTEXIST | 索引不存在 | 检查索引是否存在 |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v1.10 及以上版本

##示例##

获取集合 sample.employee 下名为“$id”的索引信息

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
     本文使用的所有引用及链接
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[faq]:manual/FAQ/faq_sdb.md
[SYSINDEXES]:manual/Manual/Catalog_Table/SYSINDEXES.md
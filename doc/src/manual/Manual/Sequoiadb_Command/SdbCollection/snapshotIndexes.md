##名称##

snapshotIndexes - 获取集合的索引信息

##语法##

**db.collectionspace.collection.snapshotIndexes([cond], [sel], [sort])**

##类别##

SdbCollection

##描述##

该函数用于获取指定集合的全部[索引][index]信息。用户通过协调节点执行该函数，将从集合相关的所有数据节点收集索引信息并聚合；通过数据节点执行该函数，将从该数据节点获取索引信息。

##参数##

- cond（ *object，选填* ）
 
    设置匹配条件及[命令位置参数][location]，默认选择所有记录

- sel（ *object，选填* ）
 
    选择返回字段名，默认值为 null ，返回所有的字段名

- sort（ *object，选填* ）
 
    对返回的记录按选定的字段排序，默认不排序，取值如下：
    
    - 1：升序
    - -1：降序

> **Note:**
>
> - sel 参数是一个 json 结构，如：{字段名:字段值}，字段值一般指定为空串。sel 中指定的字段名在记录中存在，设置字段值不生效；不存在则返回 sel 中指定的字段名和字段值。
> - 记录中字段值类型为数组，我们可以在 sel 中指定该字段名，用"."操作符加上双引号("")来引用数组元素。

##返回值##

函数执行成功时，将返回一个 SdbCursor 类型的对象。通过游标对象获取该集合的索引信息，字段说明可参考 [SYSINDEXES 集合][SYSINDEXES]。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][error_guide]。

##版本##

v3.6 及以上版本

##示例##

获取集合 sample.employee 的索引信息

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
    "Standalone": false
  },
  "IndexFlag": "Normal",
  "Type": "Positive",
  "ExtDataName": null
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
     本文使用的所有引用及链接
[index]:manual/Distributed_Engine/Architecture/Data_Model/index.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md
[SYSINDEXES]:manual/Manual/Catalog_Table/SYSINDEXES.md
[location]:manual/Manual/Sequoiadb_Command/location.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
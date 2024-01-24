##名称##

getCollectionStat - 获取集合的统计信息

##语法##

**db.collectionspace.collection.getCollectionStat()**

##类别##

SdbCollection

##描述##
 
该函数用于获取当前集合的统计信息。

##参数##

无

##返回值##

函数执行成功时，将返回一个 BSONObj 类型的对象。通过该对象获取集合的统计信息，字段说明如下:

| 字段名          | 类型    | 描述                                                   |
| --------------- | ------- | ------------------------------------------------------ |
| Collection      | string  | 集合名称                                               |
| InternalV       | uint32  | 集合统计信息的版本                                     |
| StatTimestamp   | string  | 集合统计信息的统计时间                                 |
| IsDefault       | boolean | 集合统计信息是否为默认值                               |
| IsExpired       | boolean | 集合统计信息是否过期，默认为 false                     |
| AvgNumFields    | uint64  | 集合内记录的平均字段个数，默认值为 10                  |
| SampleRecords   | uint64  | 集合内进行抽样的记录个数，默认值为 200                 |
| TotalRecords    | uint64  | 集合内记录总数，默认值为 200                           |
| TotalDataPages  | uint64  | 集合内数据页总数，默认值为 1                           |
| TotalDataSize   | uint64  | 集合内数据总大小，默认值为 80000，单位为字节           |

函数执行失败时，将抛异常并输出错误信息。

##错误##

getCollectionStat() 函数常见异常如下：

| 错误码 | 错误名                 | 报错原因                                     | 解决办法                 |
| ------ | ---------------------  | -------------------------------------------- | ------------------------ |
| -377   | SDB_MAINCL_NOIDX_NOSUB | 所操作的主集合未挂载子集合| 通过 [attachCL()][attachCL] 在该主集合上挂载任意子集合     |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.6.1 及以上版本

##示例##

获取集合 sample.employee 的统计信息

```lang-javascript
> db.sample.employee.getCollectionStat()
```

输出结果如下：

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
> 当字段 IsDefault 显示为 true 时，表示未查询到对应集合的统计信息。用户需手动执行 [analyze()][analyze] 更新集合的统计信息后，再进行统计信息的获取操作。


[^_^]:
    本文使用的引用及链接
[analyze]:manual/Manual/Sequoiadb_Command/Sdb/analyze.md
[attachCL]:manual/Manual/Sequoiadb_Command/SdbCollection/attachCL.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[faq]:manual/FAQ/faq_sdb.md
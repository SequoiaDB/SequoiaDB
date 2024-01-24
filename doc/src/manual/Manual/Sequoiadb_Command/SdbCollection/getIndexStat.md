##名称##

getIndexStat - 获取指定索引的统计信息

##语法##

**db.collectionspace.collection.getIndexStat(\<index name\>, [detail])**

##类别##

SdbCollection

##描述##

该函数用于获取当前集合中指定索引的统计信息。

##参数##

* index name ( *string，必填* )

    被指定索引的名称

* detail ( *boolean，选填* )

    是否获取索引的详细信息，默认值为 false，表示不获取

    - 详细信息将返回索引的频繁数值集合（Most Common Values，MCV）统计信息
    - 仅在 SequoiaDB v3.6.1 及以上版本中生效

##返回值##

函数执行成功时，将返回一个 BSONObj 类型的对象。通过该对象获取汇总后的索引统计信息，字段说明可参考[索引统计信息快照][SDB_SNAP_INDEXSTATS]。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`getIndexStat()` 函数常见异常如下：

|错误码|错误名|可能发生的原因|解决办法|
|------|------|--------------|--------|
|-356  |SDB_IXM_STAT_NOTEXIST|1.索引尚未被统计<br>2.索引不存在|1.通过 [db.analyze()][analyze] 接口收集统计信息<br>2.检查索引是否存在|

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.4.2 及以上版本

##示例##

获取集合 sample.employee 中 ageIndex 索引的统计信息

```lang-javascript
> db.sample.employee.getIndexStat("ageIndex")
```

输出结果如下：

```lang-json
{
  "Collection": "sample.employee",
  "Index": "ageIndex",
  "Unique": false,
  "KeyPattern": {
    "age": 1
  },
  "TotalIndexLevels": 1,
  "TotalIndexPages": 2,
  "DistinctValNum": [
    74
  ],
  "MinValue": {
    "age": 18
  },
  "MaxValue": {
    "age": 54
  },
  "NullFrac": 0,
  "UndefFrac": 0,
  "SampleRecords": 400,
  "TotalRecords": 518,
  "StatTimestamp": "2020-07-24-16.15.08.347000"
}
```


[^_^]:
    本文使用的引用及链接
[INDEXSTATS]:manual/Manual/Snapshot/SDB_SNAP_INDEXSTATS.md
[analyze]:manual/Manual/Sequoiadb_Command/Sdb/analyze.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[faq]:manual/FAQ/faq_sdb.md
[SDB_SNAP_INDEXSTATS]:manual/Manual/Snapshot/SDB_SNAP_INDEXSTATS.md

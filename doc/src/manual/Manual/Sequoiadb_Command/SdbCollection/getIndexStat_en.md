##NAME##

getIndexStat - get statistics of the specified index

##SYNOPSIS##

**db.collectionspace.collection.getIndexStat(\<index name\>, [detail])**

##CATEGORY##

SdbCollection

##DESCRIPTION##

This function is used to get statistics of the specified index.

##PARAMETERS##

* index name ( *string, required* )

    Name of the specified index.

* detail ( *boolean, optional* )

    Whether to get index details. The default value is false, which means not to get.

    - Details will return MCV(Most Common Values) statistics of the index.
    - Only valid in SequoiaDB v3.6.1 and above versions.

##RETURN VALUE##

When the function executes successfully, it will return an aggregated statistics of the index through the BSONObj. Users can refer to [SDB_SNAP_INDEXSTATS](database_management/monitoring/snapshot/SDB_SNAP_INDEXSTATS.md) to get the returned field information.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common expretions of `getIndexStat()` function are as follows:

|Error Code|Error Name|Causes|Solution|
|----------|----------|------|--------|
|-356      |SDB_IXM_STAT_NOTEXIST|1.Index has not been analyzed; <br>2.Index doesn't exist;|1.Collect statistics by [db.analyze()](reference/Sequoiadb_command/Sdb/analyze.md); <br>2.Check if the index exists;|

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the error code. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.4.2 and above

##EXAMPLES##

Get the statistics of index ageIndex of collection "sample.employee".

```lang-javascript
> db.sample.employee.getIndexStat("ageIndex")
```

The output is as follows:

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
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md

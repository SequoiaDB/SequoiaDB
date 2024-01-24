
##名称##

getDetail - 获取集合具体信息

##语法##

**db.collectionspace.collection.getDetail\(\)**

##类别##

SdbCollection

##描述##

该函数用于获取当前集合在各个数据组的具体信息。

##参数##

无

##返回值##

函数执行成功时，将返回一个 SdbCursor 类型的对象。通过该对象获取集合详细信息列表，字段说明可参考
[集合快照视图][SNAPSHOT_CL]。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.2.5 及以上版本、v3.4.1 及以上版本

##示例##

获取集合 `sample.employee` 的详细信息

```lang-javascript
> db.sample.employee.getDetail()
{
  "Name": "sample.employee",
  "UniqueID": 4294967297,
  "CollectionSpace": "sample",
  "Details": [
    {
      "NodeName": "hostname:11820",
      "GroupName": "group1",
      "InternalV": 2,
      "ID": 0,
      "LogicalID": 0,
      "Sequence": 1,
      "Indexes": 1,
      "Status": "Normal",
      "Attribute": "Compressed",
      "CompressionType": "lzw",
      "DictionaryCreated": false,
      "DictionaryVersion": 0,
      "PageSize": 65536,
      "LobPageSize": 262144,
      "TotalRecords": 0,
      "TotalLobs": 2,
      "TotalDataPages": 0,
      "TotalIndexPages": 2,
      "TotalLobPages": 2,
      "TotalUsedLobSpace": 524288,
      "UsedLobSpaceRatio": 0,
      "TotalLobSize": 300,
      "TotalValidLobSize": 300,
      "LobUsageRate": 0,
      "AvgLobSize": 150,
      "TotalDataFreeSpace": 0,
      "TotalIndexFreeSpace": 65515,
      "CurrentCompressionRatio": 1,
      "DataCommitLSN": 80,
      "IndexCommitLSN": 80,
      "LobCommitLSN": 1468,
      "DataCommitted": true,
      "IndexCommitted": true,
      "LobCommitted": true,
      "TotalDataRead": 0,
      "TotalIndexRead": 0,
      "TotalDataWrite": 0,
      "TotalIndexWrite": 0,
      "TotalUpdate": 0,
      "TotalDelete": 0,
      "TotalInsert": 0,
      "TotalSelect": 0,
      "TotalRead": 0,
      "TotalWrite": 0,
      "TotalTbScan": 1,
      "TotalIxScan": 0,
      "TotalLobGet": 0,
      "TotalLobPut": 2,
      "TotalLobDelete": 0,
      "TotalLobList": 0,
      "TotalLobReadSize": 0,
      "TotalLobWriteSize": 300,
      "TotalLobRead": 0,
      "TotalLobWrite": 2,
      "TotalLobTruncate": 0,
      "TotalLobAddressing": 2,
      "ResetTimestamp": "2022-10-06-18.04.31.090482",
      "CreateTime": "2022-10-06-18.04.31.090000",
      "UpdateTime": "2022-10-06-18.04.31.164000"
    }
  ]
}
...
```


[^_^]:
     本文使用的所有引用及链接
[SNAPSHOT_CL]:manual/Manual/SQL_Grammar/Monitoring/SNAPSHOT_CL.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[faq]:manual/FAQ/faq_sdb.md

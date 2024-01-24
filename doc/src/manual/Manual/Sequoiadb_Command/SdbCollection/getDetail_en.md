##NAME##

getDetail - get detailed information of the current collection

##SYNOPSIS##

**db.collectionspace.collection.getDetail\(\)**

##CATEGORY##

SdbCollection

##DESCRIPTION##

Get detailed information of the current collection.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, it will return a detailed list of current collection through the cursor. Users can refer to [SDB_SNAP_COLLECTIONS][SNAPSHOT_CL] to get the returned field information.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the error code. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.2.5 and above, v3.4.1 and above

##EXAMPLES##

Get the detail of collection `sample.employee`.

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
     links
[SNAPSHOT_CL]:manual/Manual/SQL_grammar/monitoring/SNAPSHOT_CL.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[faq]:manual/FAQ/faq_sdb.md

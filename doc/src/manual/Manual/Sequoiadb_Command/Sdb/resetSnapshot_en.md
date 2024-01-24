##NAME##

renameSequence - reset snapshot

##SYNOPSIS##

**db.resetSnapshot([options])**

##CATEGORY##

Sdb

##DESCRIPTION##

Clear the statistical information and restart the statistics. The statistical information mainly includes TotalDataRead, TotalDataWrite, etc.

##PARAMETERS##

| Name    | Type   | Description    | Required or Not |
|---------|--------|----------------|-----------------|
| options | object | Specify snapshot type, session ID, collection space, collection and [Command positional parameter][location].  | Not         |

**options format**

| Name | Description | Defaults | format |
| ---- | ----------- | -------- | ------ |
| Type | Specify the [type of snapshot][snapshot] to reset, the values are as follows: <br/>"sessions": snapshot of sessions<br/>"sessions current": snapshot of current sessions<br/>"database": snapshot of database <br/>"health": snapshot of node health <br/>"collections": snapshot of collections<br/>"all": all snapshots. | "all" | Type:"sessions" |
| SessionID | Specify the reset session ID, and this parameter is intended to be valid when the Type is "session". | all sessions | SessionID:1 |
| CollectionSpace | Specify the name of the collection space whose snapshot statistics needs to reset. | null | CollectionSpace:"sample" |
| Collection | Specify the name of the collection whose snapshot statistics needs to reset. | null | Collection:"sample.employee" |
| Location Elements | Command positional parameter. | all nodes | GroupName:"db1" |

> **Note:**
>
> The CollectionSpace field and the Collection field are only valid when the Type is "collections" and cannot be specified at the same time. The snapshot statistics of all collections are cleared by default, when neither field is specified.

**Type reset item**

| Snapshot Type | Reset Item |
| ------------- | ---------- |
| [sessions][SDB_SNAP_SESSIONS] | "TotalDataRead"，"TotalIndexRead"，"TotalDataWrite"，"TotalIndexWrite"<br/>"WriteTimeSpent"，"ResetTimestamp"，"LastOpType"，"LastOpBegin"<br/>"TotalRead"，"TotalReadTime"，"TotalWriteTime"，"ReadTimeSpent"<br/>"LastOpEnd"，"LastOpInfo"，"ReadTimeSpent"，"WriteTimeSpent"<br/>"TotalUpdate"，"TotalDelete"，"TotalInsert"，"TotalSelect" |
| [sessions current][SDB_SNAP_SESSIONS_CURRENT] | Same as "sessions" reset item. |
| [database][SDB_SNAP_DATABASE] | "TotalDataRead"，"TotalIndexRead"，"TotalDataWrite"<br/>"svcNetOut"，"TotalReadTime"，"TotalWriteTime"，"TotalIndexWrite"，<br/>"TotalUpdate"，"TotalDelete"，"TotalInsert"，"TotalSelect"，"TotalRead"<br/>"ReplUpdate"，"ReplInsert"，"ReplDelete"，"svcNetIn" |
| [health][SDB_SNAP_HEALTH] | "ErrNum":{"SDB_OOM"，"SDB_NOSPC"，"SDB_TOO_MANY_OPEN_FD"} |
| [collections][SDB_SNAP_COLLECTIONS] | "TotalDataRead"，"TotalIndexRead"，"TotalDataWrite"，"TotalIndexWrite"<br/>"TotalUpdate"，"TotalDelete"，"TotalInsert"，"TotalSelect"<br/>"TotalRead"，"TotalWrite"，"TotalTbScan"，"TotalIxScan"<br/>"ResetTimestamp" |
| all | In addition to resetting all the items above, it also includes "TotalTime","TotalContexts". |


##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

1.View the session snapshot with SessionID 22. 

```lang-javascript
> db.snapshot(SDB_SNAP_SESSIONS,{"SessionID":22})
{
  "NodeName": "sdbserver:31820",
  "SessionID": 22,
  "TID": 11076,
  "Status": "Waiting",
  "IsBlocked": false,
  "Type": "ShardAgent",
  "Name": "Type:Shard,NetID:1,R-TID:12930,R-IP:192.168.20.53,R-Port:11810",
  "Doing": "",
  "Source": "",
  "QueueSize": 0,
  "ProcessEventCount": 32,
  "RelatedID": "c0a814352e220000000000000016",
  "Contexts": [
      200
  ],
  "TotalDataRead": 27577,
  "TotalIndexRead": 0,
  "TotalDataWrite": 0,
  "TotalIndexWrite": 0,
  "TotalUpdate": 0,
  "TotalDelete": 0,
  "TotalInsert": 0,
  "TotalSelect": 27577,
  "TotalRead": 27577,
  "TotalReadTime": 0,
  "TotalWriteTime": 0,
  "ReadTimeSpent": 0,
  "WriteTimeSpent": 0,
  "ConnectTimestamp": "2019-06-20-13.55.52.646730",
  "ResetTimestamp": "2019-06-20-13.55.52.646730",
  "LastOpType": "GETMORE",
  "LastOpBegin": "--",
  "LastOpEnd": "2019-06-20-14.20.22.223637",
  "LastOpInfo": "ContextID:200, NumToRead:-1",
  "UserCPU": 0.38,
  "SysCPU": 0.29
}
```

2.Reset snapshot.

```lang-javascript
> db.resetSnapshot({Type:"sessions",SessionID:22})
```

3.View the session snapshot after reset.

```lang-javascript
> db.snapshot(SDB_SNAP_SESSIONS,{"SessionID":22})
{
  "NodeName": "sdbserver:31820",
  "SessionID": 22,
  "TID": 11076,
  "Status": "Waiting",
  "IsBlocked": false,
  "Type": "ShardAgent",
  "Name": "Type:Shard,NetID:1,R-TID:12930,R-IP:192.168.20.53,R-Port:11810",
  "Doing": "",
  "Source": "",
  "QueueSize": 0,
  "ProcessEventCount": 32,
  "RelatedID": "c0a814352e220000000000000016",
  "Contexts": [
      200
  ],
  "TotalDataRead": 0,
  "TotalIndexRead": 0,
  "TotalDataWrite": 0,
  "TotalIndexWrite": 0,
  "TotalUpdate": 0,
  "TotalDelete": 0,
  "TotalInsert": 0,
  "TotalSelect": 0,
  "TotalRead": 0,
  "TotalReadTime": 0,
  "TotalWriteTime": 0,
  "ReadTimeSpent": 0,
  "WriteTimeSpent": 0,
  "ConnectTimestamp": "2019-06-20-13.55.52.646730",
  "ResetTimestamp": "2019-06-20-14.23.42.059988",
  "LastOpType": "UNKNOW",
  "LastOpBegin": "--",
  "LastOpEnd": "--",
  "LastOpInfo": "",
  "UserCPU": 0.38,
  "SysCPU": 0.3
}
```

[^_^]:
     links
[location]:manual/Manual/Sequoiadb_Command/location.md
[snapshot]:manual/Manual/Snapshot/snapshot.md
[SDB_SNAP_SESSIONS]:manual/Manual/Snapshot/SDB_SNAP_SESSIONS.md
[SDB_SNAP_SESSIONS_CURRENT]:manual/Manual/Snapshot/SDB_SNAP_SESSIONS_CURRENT.md
[SDB_SNAP_DATABASE]:manual/Manual/Snapshot/SDB_SNAP_DATABASE.md
[SDB_SNAP_HEALTH]:manual/Manual/Snapshot/SDB_SNAP_HEALTH.md
[SDB_SNAP_COLLECTIONS]:manual/Manual/Snapshot/SDB_SNAP_COLLECTIONS.md
[getLastErrObj]:manual/Manual/Sequoiadb_Command/Global/getLastErrObj.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md

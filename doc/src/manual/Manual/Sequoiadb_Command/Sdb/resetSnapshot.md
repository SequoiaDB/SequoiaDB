##名称##

resetSnapshot - 重置快照

##语法##

**db.resetSnapshot([options])**

##类别##

Sdb

##描述##

该函数用于清空之前的统计信息，使统计重新进行。主要针对的统计信息包括 TotalDataRead、TotalDataWrite 等。

##参数##

| 参数名  | 参数类型 | 描述   | 是否必填 |
| ------- | -------- | ------ | -------- |
| options | object   | 指定快照类型、会话 ID、集合空间、集合、[命令位置参数][location] | 否 |

**options 格式**

| 属性名 | 描述   | 默认值 | 格式 |
| ------ | ------ | -------| ---- |
| Type   | 指定重置的[快照类型][snapshot]，取值如下：<br/>"sessions"：会话快照<br/>"sessions current"：当前会话快照<br/>"database"：数据库快照<br/>"health"：节点健康检测快照<br/>"collections"：集合快照<br/>"all"：所有快照 | "all" | Type:"sessions" |
| SessionID | 指定重置的会话 ID，该参数只在 Type 为"sessions"时有效 | 所有会话 | SessionID: 1 |
| CollectionSpace | 指定需要重置快照统计信息的集合空间名称 | 空 | CollectionSpace:"sample" |
| Collection | 指定需要重置快照统计信息的集合名称 | 空 | Collection:"sample.employee" |
| Location Elements | 命令位置参数 | 所有节点 | GroupName:"db1" |

> **Note:**
>
> CollectionSpace 字段和 Collection 字段只在 Type 为"collections"时有效，且不能同时指定；两个字段都不指定时，默认清空所有集合的快照统计信息。

**Type 重置项**

| 快照类型 | 重置项  |
| ------ | ------ |
| [sessions][SDB_SNAP_SESSIONS] | "TotalDataRead"，"TotalIndexRead"，"TotalDataWrite"，"TotalIndexWrite"<br/>"WriteTimeSpent"，"ResetTimestamp"，"LastOpType"，"LastOpBegin"<br/>"TotalRead"，"TotalReadTime"，"TotalWriteTime"，"ReadTimeSpent"<br/>"LastOpEnd"，"LastOpInfo"，"ReadTimeSpent"，"WriteTimeSpent"<br/>"TotalUpdate"，"TotalDelete"，"TotalInsert"，"TotalSelect" |
| [sessions current][SDB_SNAP_SESSIONS_CURRENT] | 与"sessions"重置项相同 |
| [database][SDB_SNAP_DATABASE] | "TotalDataRead"，"TotalIndexRead"，"TotalDataWrite"<br/>"svcNetOut"，"TotalReadTime"，"TotalWriteTime"，"TotalIndexWrite"，<br/>"TotalUpdate"，"TotalDelete"，"TotalInsert"，"TotalSelect"，"TotalRead"<br/>"ReplUpdate"，"ReplInsert"，"ReplDelete"，"svcNetIn" |
| [health][SDB_SNAP_HEALTH] | "ErrNum":{"SDB_OOM"，"SDB_NOSPC"，"SDB_TOO_MANY_OPEN_FD"} |
| [collections][SDB_SNAP_COLLECTIONS] | "TotalDataRead"，"TotalIndexRead"，"TotalDataWrite"，"TotalIndexWrite"<br/>"TotalUpdate"，"TotalDelete"，"TotalInsert"，"TotalSelect"<br/>"TotalRead"，"TotalWrite"，"TotalTbScan"，"TotalIxScan"<br/>"ResetTimestamp" |
| all | 除了重置上述所有的项，还包括：<br/>"TotalTime"，"TotalContexts" |

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛出异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.0 及以上版本

##示例##

1. 查看 SessionID 为 22 的会话快照

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
  
2. 重置快照

    ```lang-javascript
    > db.resetSnapshot({Type:"sessions",SessionID:22})
    ```

3. 查看重置后的会话快照
  
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
    本文使用的所有引用及链接
[location]:manual/Manual/Sequoiadb_Command/location.md
[snapshot]:manual/Manual/Snapshot/Readme.md
[SDB_SNAP_SESSIONS]:manual/Manual/Snapshot/SDB_SNAP_SESSIONS.md
[SDB_SNAP_SESSIONS_CURRENT]:manual/Manual/Snapshot/SDB_SNAP_SESSIONS_CURRENT.md
[SDB_SNAP_DATABASE]:manual/Manual/Snapshot/SDB_SNAP_DATABASE.md
[SDB_SNAP_HEALTH]:manual/Manual/Snapshot/SDB_SNAP_HEALTH.md
[SDB_SNAP_COLLECTIONS]:manual/Manual/Snapshot/SDB_SNAP_COLLECTIONS.md
[getLastErrObj]:manual/Manual/Sequoiadb_Command/Global/getLastErrObj.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md


服务任务快照可以列出当前数据库节点中服务任务的统计信息，输出一条记录。

##标识##

SDB_SNAP_SVCTASKS

##字段信息##

| 字段名            | 类型          | 描述                                               |
| ----------------- | ------------- | -------------------------------------------------- |
| NodeName          | string        | 任务所在的节点                                     |
| TaskName          | string        | 任务名称                                           |
| TaskID            | int32         | 任务 ID                                            |
| Time              | int64         | 任务持续时间                                       |
| TotalContexts     | int64         | 上下文记录总数量                                   |
| TotalDataRead     | int64         | 读取的数据记录总数量，包含溢出记录                 |
| TotalIndexRead    | int64         | 索引被读的次数                                     |
| TotalDataWrite    | int64         | 数据记录写                                         |
| TotalIndexWrite   | int64         | 索引写                                             |
| TotalUpdate       | int64         | 总更新记录数量                                     |
| TotalDelete       | int64         | 总删除记录数量                                     |
| TotalInsert       | int64         | 总插入记录数量                                     |
| TotalSelect       | int64         | 总选取记录数量                                     |
| TotalRead         | int64         | 读取的数据记录总数量，不包含溢出记录               |
| TotalWrite        | int64         | 总数据写                                           |
| StartTimestamp    | string        | 开始时间                                           |
| ResetTimestamp    | string        | 重置时间                                           |

##示例##

查看服务任务快照

```lang-javascript
> db.snapshot( SDB_SNAP_SVCTASKS )
```

输出结果如下：

```lang-json
{
  "NodeName": "u1604-ljh:42000",
  "TaskID": 0,
  "TaskName": "Default",
  "Time": 4271,
  "TotalContexts": 62,
  "TotalDataRead": 0,
  "TotalIndexRead": 0,
  "TotalDataWrite": 0,
  "TotalIndexWrite": 0,
  "TotalUpdate": 0,
  "TotalDelete": 0,
  "TotalInsert": 0,
  "TotalSelect": 25,
  "TotalRead": 0,
  "TotalWrite": 0,
  "StartTimestamp": "2019-08-14-10.30.59.172628",
  "ResetTimestamp": "2019-08-14-10.30.59.172628"
}
...
```
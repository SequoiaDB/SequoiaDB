
查询快照可以列出数据库中正在进行的查询信息。

>**Note:**
>
> 每一个数据节点上正在进行的每一个查询操作为一条记录。

##标识##

$SNAPSHOT_QUERIES

##协调节点字段信息##

| 字段名                 | 类型     | 描述                                            |
| ---------------------- | -------- | ----------------------------------------------- |
| NodeName               | string   | 节点名，格式为\<hostname\>:\<servicename\>      |
| NodeID                 | bson array | 节点的 ID，格式为[<分区组 ID>,<节点 ID>]      |
| StartTimestamp         | string   | 查询开始时间                                    |
| EndTimestamp           | string   | 查询结束时间                                    |
| TID                    | int32    | 内部线程 ID                                     |
| OpType                 | string   | 操作类型                                        |
| Name                   | string   | 操作对象名                                      |
| QueryTimeSpent         | int32    | 查询总共花费时间，单位为毫秒                    |
| ReturnNum              | int32    | 返回值                                          |
| TotalMsgSent           | int32    | 发送到远程节点的消息总数                        |
| LastOpInfo             | string   | 查询语句内容                                    |
| MsgSentTime            | int32    | 消息发送花费时间，单位为毫秒                    |
| RemoteNodeWaitTime     | int32    | 等待远程节点花费时间，单位为毫秒                |
| ClientInfo             | bson     | 连接到SequoiaDB引擎执行该查询的客户端信息       |
| RelatedNode            | bson array | 处理该查询时经由该协调节点发送到的远程数据节点集|

##数据节点字段信息##

| 字段名                 | 类型     | 描述                                                                                     |
| ---------------------- | -------- | ---------------------------------------------------------------------------------------- |
| NodeName               | string   | 节点名，格式为\<hostname\>:\<servicename\>                                               |
| NodeID                 | bson array | 节点的 ID，格式为[<分区组 ID>,<节点 ID>]                                               |
| StartTimestamp         | string   | 查询开始时间                                                                             |
| EndTimestamp           | string   | 查询结束时间                                                                             |
| TID                    | int32    | 内部线程 ID                                                                              |
| OpType                 | string   | 操作类型                                                                                 |
| Name                   | string   | 操作对象名                                                                               |
| QueryTimeSpent         | int32    | 查询总共花费时间，单位为毫秒                                                             |
| ReturnNum              | int32    | 返回值                                                                                   |
| RelatedNID             | int32    | 将该查询请求发送到该数据节点的的相关协调节点 ID                                           |
| RelatedTID             | int32    | 发送查询的相关协调节点的线程 ID，结合 RelatedNID 可以将协调节点和数据节点的快照输出联系起来 |
| SessionID              | int32    | 内部会话ID                                                                               |
| AccessPlanID           | int32    | 访问计划 ID                                                                              |
| DataRead               | int32    | 数据记录读                                                                               |
| DataWrite              | int32    | 数据记录写                                                                               |
| IndexRead              | int32    | 索引读                                                                                   |
| IndexWrite             | int32    | 索引写                                                                                   |
| LobRead                | int32    | 服务端中 LOB 分片的读次数 |
| LobWrite               | int32    | 服务端中 LOB 分片的写次数  |
| LobTruncate    | int64     | 服务端中 LOB 分片的截断次数（仅在 v3.6.1 及以上版本生效） |
| LobAddressing     | int64     | 服务端中 LOB 分片的寻址总次数（仅在 v3.6.1 及以上版本生效） |
| TransLockWaitTime      | int32    | 锁等待时间，单位为毫秒                                                                   |
| LatchWaitTime          | int32    | 闩锁等待时间，单位为毫秒                                                                 |

###ClientInfo 字段信息###

| 字段名                 | 类型     | 描述                                                                   |
| ---------------------- | -------- | ---------------------------------------------------------------------- |
| ClientTID              | int32    | 连接协调节点客户端线程 ID                                              |
| ClientHost             | int32    | 连接协调节点客户端所在主机 IP                                          |
| ClientPort             | int32    | 连接协调节点客户端所在主机端口，只有当连接客户端为 SQL 引擎时才会显示  |
| ClientQID              | int32    | 连接协调节点客户端程序查询 ID，只有当连接客户端为 SQL 引擎时才会显示   |

##示例##

- 通过协调节点获取查询快照

    ```lang-javascript
    > db.exec("select * from $SNAPSHOT_QUERIES")
    ```

    输出结果如下：

    ```lang-json
    {
      "NodeName": "sdbserver:50000",
      "NodeID": [
        2,
        4
      ],
      "StartTimestamp": "2020-06-12-11.33.14.019931",
      "EndTimestamp": "1970-01-01-08.00.00.000000",
      "TID": 10832,
      "OpType": "QUERY",
      "Name": "sbtest1.sbtest2",
      "QueryTimeSpent": 0,
      "ReturnNum": 0,
      "TotalMsgSent": 1,
      "LastOpInfo": "Collection:sbtest1.sbtest2, Matcher:{ \"id\": { \"$et\": 5015 } }, Selector:{}, OrderBy:{ \"id\": 1 }, Hint:{ \"\": \"PRIMARY\" }, Skip:0, Limit:-1, Flag:0x00000200(512)",
      "MsgSentTime": 0.034,
      "RemoteNodeWaitTime": 0,
      "ClientInfo": {
        "ClientTID": 24343,
        "ClientHost": "192.168.56.101"
      },
      "RelatedNode": [
        1002
      ]
    }
    ```

- 通过数据节点获取查询快照

    ```lang-javascript
    > db.exec("select * from $SNAPSHOT_QUERIES")
    ```

    输出结果如下：

    ```lang-json
    {
      "NodeName": "sdbserver:42000",
      "NodeID": [
        1000,
        1002
      ],
      "StartTimestamp": "2020-06-12-11.29.44.906939",
      "EndTimestamp": "1970-01-01-08.00.00.000000",
      "TID": 10850,
      "OpType": "QUERY",
      "Name": "$snapshot queries",
      "QueryTimeSpent": 0.118,
      "ReturnNum": 0,
      "RelatedNID": 0,
      "RelatedTID": 0,
      "SessionID": 47,
      "AccessPlanID": -1,
      "DataRead": 0,
      "DataWrite": 0,
      "IndexRead": 0,
      "IndexWrite": 0,
      "LobRead": 0,
      "LobWrite": 0,
      "LobTruncate": 0,
      "LobAddressing": 0,
      "TransLockWaitTime": 0,
      "LatchWaitTime": 0
    }
    ```


集合快照可以列出当前数据库节点中所有的非临时集合（协调节点中列出用户集合），每个集合为一条记录。

##标识##

$SNAPSHOT_CL

##字段信息##

| 字段名              | 类型          | 描述                                                    |
| ------------------- | ------------- | ------------------------------------------------------- |
| Name                | string        | 集合完整名                                              |
| UniqueID            | int64         | 集合的 UniqueID，在集群上全局唯一                        |
| CollectionSpace     | string        | 集合所属集合空间名                                      |
| Details.NodeName            | string        | 集合所属节点名，格式为<主机名>:<端口号>                        |
| Details.GroupName           | string        | 集合所属复制组名                                        |
| Details.InternalV           | int32         | 集合视图的版本                                  |
| Details.ID          | int32         | 集合 ID，范围 0 ~ 4095，集合空间内唯一                  |
| Details.LogicalID   | int32         | 集合逻辑 ID                                             |
| Details.Sequence    | int32         | 序列号                                                  |
| Details.Indexes     | int32         | 该集合所包含的索引数量                                  |
| Details.Status      | string        | 集合当前状态，取值如下：<br> "Free"：空闲<br>"Normal"：正常<br>"Dropped"：被删除<br> "Offline Reorg Shadow Copy Phase"：离线重组复制阶段<br> "Offline Reorg Truncate Phase"：离线重组清除阶段<br> "Offline Reorg Copy Back Phase"：离线重组重入阶段<br> "Offline Reorg Rebuild Phase"：离线重组重建索引阶段 |
| Details.Attribute   |  string       | 属性 |
| Details.CompressionType | string    | 压缩类型 |
| Details.DictionaryCreated | boolean | 是否创建压缩字典  |
| Details.DictionaryVersion   | int32 | 压缩字典版本 |
| Details.PageSize            | int32 | 集合页的大小  |
| Details.LobPageSize         | int32 | 大对象页的大小   |
| Details.TotalRecords        | int64         | 集合的记录总数                                          |
| Details.TotalLobs           | int64         | 集合大对象文件总数  |
| Details.TotalDataPages      | int32         | 集合的数据页总数                                        |
| Details.TotalIndexPages     | int32         | 集合的索引页总数                                        |
| Details.TotalLobPages       | int32         | 集合大对象文件已使用空间数据页个数                      |
| Details.TotalUsedLobSpace   | int64      | 集合空间大对象文件已使用的空间大小，单位为字节（仅在 v3.6.1 及以上版本生效） |
| Details.UsedLobSpaceRatio    | double      | 集合大对象文件已使用的空间占其所在集合空间存储容量的比率（仅在 v3.6.1 及以上版本生效） |
| Details.TotalLobSize     | int64      | 集合大对象文件的数据总大小，单位为字节（仅在 v3.6.1 及以上版本生效）    |
| Details.TotalValidLobSize | int64     | 集合空间大对象文件有效数据总大小，单位为字节（仅在 v3.6.1 及以上版本生效） |
| Details.LobUsageRate    | double  | 集合空间大对象文件的有效使用率（仅在 v3.6.1 及以上版本生效）<br>使用率越高，空间浪费越少 |
| Details.AvgLobSize     | int64      | 集合空间大对象文件平均大小，单位为字节（仅在 v3.6.1 及以上版本生效） |
| Details.TotalDataFreeSpace  | int64         | 集合的数据空闲空间，单位为字节                        |
| Details.TotalIndexFreeSpace | int64         | 集合的索引空闲空间，单位为字节                        |
| Details.CurrentCompressionRatio | double    | 集合的的压缩率                                          |
| Details.DataCommitLSN   | int64      | 集合数据文件最后提交LSN    |
| Details.IndexCommitLSN  | int64      | 集合索引文件最后提交LSN    |
| Details.LobCommitLSN    | int64      | 集合大对象文件最后提交LSN  |
| Details.DataCommitted   | boolean    | 集合数据文件当前是否有效提交 |
| Details.IndexCommitted  | boolean    | 集合索引文件当前是否有效提交 |
| Details.LobCommitted    | boolean    | 集合大对象文件当前是否有效提交 |
| Details.TotalDataRead   | int64  | 集合数据读请求 |
| Details.TotalIndexRead  | int64  | 集合索引读请求 |
| Details.TotalDataWrite  | int64  | 集合数据写请求 |
| Details.TotalIndexWrite | int64  | 集合索引写请求 |
| Details.TotalUpdate     | int64  | 集合更新记录数量 |
| Details.TotalDelete     | int64  | 集合删除记录数量 |
| Details.TotalInsert     | int64  | 集合插入记录数量 |
| Details.TotalSelect     | int64  | 集合选择记录数量 |
| Details.TotalRead       | int64  | 集合读取记录数量 |
| Details.TotalWrite      | int64  | 集合写入记录数量 |
| Details.TotalTbScan     | int64  | 集合使用表扫描次数 |
| Details.TotalIxScan     | int64  | 集合使用索引扫描次数 |
| Details.TotalLobGet           | int64     | 客户端获取大对象文件的总次数（仅在 v3.6.1 及以上版本生效） |
| Details.TotalLobPut           | int64     | 客户端上传大对象文件的总次数（仅在 v3.6.1 及以上版本生效） |
| Details.TotalLobDelete        | int64     | 客户端删除大对象文件的总次数（仅在 v3.6.1 及以上版本生效） |
| Details.TotalLobList          | int64     | 客户端列举大对象文件的总次数（仅在 v3.6.1 及以上版本生效） |
| Details.TotalLobReadSize      | int64     | 客户端读大对象文件的总字节数（仅在 v3.6.1 及以上版本生效） |
| Details.TotalLobWriteSize     | int64     | 客户端写大对象文件的总字节数（仅在 v3.6.1 及以上版本生效） |
| Details.TotalLobRead     | int64     | 服务端中 LOB 分片的读次数（仅在 v3.6.1 及以上版本生效） |
| Details.TotalLobWrite     | int64     | 服务端中 LOB 分片的写次数（仅在 v3.6.1 及以上版本生效） |
| Details.TotalLobTruncate    | int64     | 服务端中 LOB 分片的截断次数（仅在 v3.6.1 及以上版本生效） |
| Details.TotalLobAddressing     | int64     | 服务端中 LOB 分片的寻址总次数（仅在 v3.6.1 及以上版本生效） |
| Details.ResetTimestamp  | string | 重置快照的时间 |
| Details.CreateTime | string | 创建集合的时间（仅在 v3.6.1 及以上版本生效） |
| Details.UpdateTime | string | 更新集合元数据的时间（仅在 v3.6.1 及以上版本生效） |

##示例##

查看集合快照

```lang-javascript
> db.exec( "select * from $SNAPSHOT_CL" )
```

输出结果如下：

```lang-json
{
  "Name": "sample.employee",
  "UniqueID": 4294967297,
  "CollectionSpace": "sample",
  "Details": [
    {
      "NodeName": "hostname:20000",
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
      "TotalLobPut": 0,
      "TotalLobDelete": 0,
      "TotalLobList": 2,
      "TotalLobReadSize": 0,
      "TotalLobWriteSize": 0,
      "TotalLobRead": 4,
      "TotalLobWrite": 0,
      "TotalLobTruncate": 0,
      "TotalLobAddressing": 4,
      "ResetTimestamp": "2022-10-09-09.34.53.288169",
      "CreateTime": "2022-10-06-18.04.31.090000",
      "UpdateTime": "2022-10-06-18.04.31.164000"
    }
  ]
}
...
```

[^_^]: 

    数据库快照
    作者：何嘉文
    时间：20190307
    评审意见
    
    王涛：
    许建辉：
    市场部：20190425


集合快照： 

- 连接非协调节点，列出所有集合（不含临时的集合）
- 连接协调节点，列出所有集合（不含临时的集合和系统的集合）

标识
----

SDB_SNAP_COLLECTIONS

非协调节点字段信息
----

| 字段名           | 类型      | 描述                              |
| ---------------- | --------- | --------------------------------- |
| Name             | string    | 集合完整名                        |
| UniqueID         | int64     | 集合的 UniqueID，在集群上全局唯一 |
| CollectionSpace  | string    | 集合所属集合空间名                |
| Details.NodeName         | string    | 集合所属节点名，格式为<主机名>:<服务名> |
| Details.GroupName        | string    | 集合所属复制组名                  |
| Details.InternalV        | int32     | 集合快照的版本                    |
| Details.ID                      | int32         | 集合 ID，范围 0~4095，集合空间内唯一                    |
| Details.LogicalID               | int32         | 集合逻辑 ID                                             |
| Details.Sequence                | int32         | 序列号                                                  |
| Details.Indexes                 | int32         | 该集合所包含的索引数量                                  |
| Details.Status                  | string        | 集合当前状态，取值如下：<br>"Free"：空闲<br>"Normal"：正常<br>"Dropped"：被删除<br>"Offline Reorg Shadow Copy Phase"：离线重组复制阶段<br>"Offline Reorg Truncate Phase"：离线重组清除阶段<br>"Offline Reorg Copy Back Phase"：离线重组重入阶段<br>"Offline Reorg Rebuild Phase"：离线重组重建索引阶段 |
| Details.Attribute               | string        | 集合的属性，取值可参考 [SYSCOLLECTION 集合][syscollection]的字段 AttributeDesc    |
| Details.CompressionType         | string        | 压缩类型，如："snappy"、"lzw"                           |
| Details.DictionaryCreated       | boolean       | 是否创建压缩字典                                        |
| Details.DictionaryVersion       | int32         | 压缩字典版本                                            |
| Details.PageSize                | int32         | 集合页的大小                                            |
| Details.LobPageSize             | int32         | 大对象页的大小                                          |
| Details.TotalRecords            | int64         | 集合的记录总数                                          |
| Details.TotalLobs               | int64         | 集合大对象文件总数 |
| Details.TotalDataPages          | int32         | 集合的数据页总数                                        |
| Details.TotalIndexPages         | int32         | 集合的索引页总数                                        |
| Details.TotalLobPages           | int32         | 集合大对象文件已使用空间数据页个数 |
| Details.TotalUsedLobSpace   | int64      | 集合大对象文件已使用的空间大小，单位为字节（仅在 v3.6.1 及以上版本生效） |
| Details.UsedLobSpaceRatio    | double      | 集合大对象文件已使用的空间占其所在集合空间存储容量的比率（仅在 v3.6.1 及以上版本生效） |
| Details.TotalLobSize     | int64      | 集合大对象文件的数据总大小，单位为字节（仅在 v3.6.1 及以上版本生效）    |
| Details.TotalValidLobSize | int64     | 集合大对象文件有效数据总大小，单位为字节（仅在 v3.6.1 及以上版本生效） |
| Details.LobUsageRate    | double  | 集合大对象文件的有效使用率（仅在 v3.6.1 及以上版本生效）<br>使用率越高，空间浪费越少 |
| Details.AvgLobSize     | int64      | 集合大对象文件平均大小，单位为字节（仅在 v3.6.1 及以上版本生效） |
| Details.TotalDataFreeSpace      | int64         | 集合的数据空闲空间，单位为字节                          |
| Details.TotalIndexFreeSpace     | int64         | 集合的索引空闲空间，单位为字节                          |
| Details.CurrentCompressionRatio | double        | 集合的的压缩率                                          |
| Details.DataCommitLSN           | int64         | 集合数据文件最后提交 LSN                                |
| Details.IndexCommitLSN          | int64         | 集合索引文件最后提交 LSN                                |
| Details.LobCommitLSN            | int64         | 集合大对象文件最后提交 LSN                              |
| Details.DataCommitted           | boolean       | 集合数据文件当前是否有效提交                            |
| Details.IndexCommitted          | boolean       | 集合索引文件当前是否有效提交                            |
| Details.LobCommitted            | boolean       | 集合大对象文件当前是否有效提交                          |
| Details.TotalDataRead   | int64 | 集合数据读请求 |
| Details.TotalIndexRead  | int64 | 集合索引读请求 |
| Details.TotalDataWrite  | int64 | 集合数据写请求 |
| Details.TotalIndexWrite | int64 | 集合索引写请求 |
| Details.TotalUpdate     | int64 | 集合更新记录数量 |
| Details.TotalDelete     | int64 | 集合删除记录数量 |
| Details.TotalInsert     | int64 | 集合插入记录数量 |
| Details.TotalSelect     | int64 | 集合选择记录数量 |
| Details.TotalRead       | int64 | 集合读取记录数量 |
| Details.TotalWrite      | int64 | 集合写入记录数量 |
| Details.TotalTbScan     | int64 | 集合使用表扫描次数 |
| Details.TotalIxScan     | int64 | 集合使用索引扫描次数 |
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

协调节点字段信息
----

| 字段名    | 类型      | 描述                               |
| --------- | --------- | ---------------------------------- |
| Name      | string    | 集合完整名                         |
| UniqueID  | int64     | 集合的 UniqueID，在集群上全局唯一  |
| Details.GroupName  | string    | 节点所在复制组名  |
| Details.Group.ID                  | int32         | 集合 ID，范围 0~4096，集合空间内唯一                    |
| Details.Group.LogicalID           | int32         | 集合逻辑 ID                                             |
| Details.Group.Sequence            | int32         | 序列号                                                  |
| Details.Group.Indexes             | int32         | 该集合所包含的索引数量                                  |
| Details.Group.Status              | string        | 集合当前状态，取值如下：<br>"Free"：空闲<br>"Normal"：正常<br>"Dropped"：被删除<br>"Offline Reorg Shadow Copy Phase"：离线重组复制阶段<br>"Offline Reorg Truncate Phase"：离线重组清除阶段<br>"Offline Reorg Copy Back Phase"：离线重组重入阶段<br>"Offline Reorg Rebuild Phase"：离线重组重建索引阶段 |
| Details.Group.TotalRecords        | int64         | 集合的记录总数                                          |
| Details.Group.TotalLobs               | int64         | 集合大对象文件总数 |
| Details.Group.TotalDataPages      | int32         | 集合的数据页总数                                        |
| Details.Group.TotalIndexPages     | int32         | 集合的索引页总数                                        |
| Details.Group.TotalLobPages           | int32         | 集合大对象文件已使用空间数据页个数 |
| Details.Group.TotalUsedLobSpace   | int64      | 集合空间大对象文件已使用的空间大小，单位为字节（仅在 v3.6.1 及以上版本生效） |
| Details.Group.UsedLobSpaceRatio    | double      | 集合大对象文件已使用的空间占其所在集合空间存储容量的比率（仅在 v3.6.1 及以上版本生效） |
| Details.Group.TotalLobSize     | int64      | 集合大对象文件的数据总大小，单位为字节（仅在 v3.6.1 及以上版本生效）    |
| Details.Group.TotalValidLobSize | int64     | 集合空间大对象文件有效数据总大小，单位为字节（仅在 v3.6.1 及以上版本生效） |
| Details.Group.LobUsageRate    | double  | 集合空间大对象文件的有效使用率（仅在 v3.6.1 及以上版本生效）<br>使用率越高，空间浪费越少 |
| Details.Group.AvgLobSize     | int64      | 集合空间大对象文件平均大小，单位为字节（仅在 v3.6.1 及以上版本生效） |
| Details.Group.TotalDataFreeSpace  | int64         | 集合的数据空闲空间，单位为字节                          |
| Details.Group.TotalIndexFreeSpace | int64         | 集合的索引空闲空间，单位为字节                          |
| Details.Group.TotalDataRead   | int64 | 集合数据读请求 |
| Details.Group.TotalIndexRead  | int64 | 集合索引读请求 |
| Details.Group.TotalDataWrite  | int64 | 集合数据写请求 |
| Details.Group.TotalIndexWrite | int64 | 集合索引写请求 |
| Details.Group.TotalUpdate     | int64 | 集合更新记录数量 |
| Details.Group.TotalDelete     | int64 | 集合删除记录数量 |
| Details.Group.TotalInsert     | int64 | 集合插入记录数量 |
| Details.Group.TotalSelect     | int64 | 集合选择记录数量 |
| Details.Group.TotalRead       | int64 | 集合读取记录数量 |
| Details.Group.TotalWrite      | int64 | 集合写入记录数量 |
| Details.Group.TotalTbScan     | int64 | 集合使用表扫描次数 |
| Details.Group.TotalIxScan     | int64 | 集合使用索引扫描次数 |
| Details.Group.TotalLobGet           | int64     | 客户端获取大对象文件的总次数（仅在 v3.6.1 及以上版本生效） |
| Details.Group.TotalLobPut           | int64     | 客户端上传大对象文件的总次数（仅在 v3.6.1 及以上版本生效） |
| Details.Group.TotalLobDelete        | int64     | 客户端删除大对象文件的总次数（仅在 v3.6.1 及以上版本生效） |
| Details.Group.TotalLobList          | int64     | 客户端列举大对象文件的总次数（仅在 v3.6.1 及以上版本生效） |
| Details.Group.TotalLobReadSize      | int64     | 客户端读大对象文件的总字节数（仅在 v3.6.1 及以上版本生效） |
| Details.Group.TotalLobWriteSize     | int64     | 客户端写大对象文件的总字节数（仅在 v3.6.1 及以上版本生效） |
| Details.Group.TotalLobRead     | int64     | 服务端中 LOB 分片的读次数（仅在 v3.6.1 及以上版本生效） |
| Details.Group.TotalLobWrite     | int64     | 服务端中 LOB 分片的写次数（仅在 v3.6.1 及以上版本生效） |
| Details.Group.TotalLobTruncate    | int64     | 服务端中 LOB 分片的截断次数（仅在 v3.6.1 及以上版本生效） |
| Details.Group.TotalLobAddressing     | int64     | 服务端中 LOB 分片的寻址总次数（仅在 v3.6.1 及以上版本生效） |
| Details.Group.ResetTimestamp  | string | 重置快照的时间 |
| Details.Group.NodeName            | string        | 节点名，格式为<主机名>:<服务名>                         |
| Details.CreateTime | string | 创建集合的时间（仅在 v3.6.1 及以上版本生效） |
| Details.UpdateTime | string | 更新集合元数据的时间（仅在 v3.6.1 及以上版本生效） |


示例
----

- 通过非协调节点查看快照

   ```lang-javascript
   > db.snapshot( SDB_SNAP_COLLECTIONS )
   ```
   
   输出结果如下：

   ```lang-json
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
         "TotalLobs": 1,
         "TotalDataPages": 0,
         "TotalIndexPages": 2,
         "TotalLobPages": 1,
         "TotalUsedLobSpace": 262144,
         "UsedLobSpaceRatio": 0,
         "TotalLobSize": 150,
         "TotalValidLobSize": 150,
         "LobUsageRate": 0,
         "AvgLobSize": 150,
         "TotalDataFreeSpace": 0,
         "TotalIndexFreeSpace": 65515,
         "CurrentCompressionRatio": 1,
         "DataCommitLSN": 80,
         "IndexCommitLSN": 80,
         "LobCommitLSN": 164,
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
         "TotalTbScan": 0,
         "TotalIxScan": 0,
         "TotalLobGet": 0,
         "TotalLobPut": 1,
         "TotalLobDelete": 0,
         "TotalLobList": 0,
         "TotalLobReadSize": 0,
         "TotalLobWriteSize": 150,
         "TotalLobRead": 0,
         "TotalLobWrite": 1,
         "TotalLobTruncate": 0,
         "TotalLobAddressing": 1,
         "ResetTimestamp": "2022-10-06-18.04.31.090482",
         "CreateTime": "2022-10-06-18.04.31.090000",
         "UpdateTime": "2022-10-06-18.04.31.164000"
       }
     ]
   }
   ...
   ```

- 通过协调节点查看快照

   ```lang-javascript
   > db.snapshot( SDB_SNAP_COLLECTIONS )
   ```

   输出结果如下：

   ```lang-json
   {
     "Name": "sample.employee",
     "UniqueID": 4294967297,
     "Details": [
       {
         "GroupName": "group1",
         "Group": [
           {
             "ID": 0,
             "LogicalID": 0,
             "Sequence": 1,
             "Indexes": 1,
             "Status": "Normal",
             "TotalRecords": 0,
             "TotalLobs": 1,
             "TotalDataPages": 0,
             "TotalIndexPages": 2,
             "TotalLobPages": 1,
             "TotalUsedLobSpace": 262144,
             "UsedLobSpaceRatio": 0,
             "TotalLobSize": 150,
             "TotalValidLobSize": 150,
             "LobUsageRate": 0,
             "AvgLobSize": 150,
             "TotalDataFreeSpace": 0,
             "TotalIndexFreeSpace": 65515,
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
             "TotalTbScan": 0,
             "TotalIxScan": 0,
             "TotalLobGet": 0,
             "TotalLobPut": 1,
             "TotalLobDelete": 0,
             "TotalLobList": 0,
             "TotalLobReadSize": 0,
             "TotalLobWriteSize": 150,
             "TotalLobRead": 0,
             "TotalLobWrite": 1,
             "TotalLobTruncate": 0,
             "TotalLobAddressing": 1,
             "ResetTimestamp": "2022-10-06-18.04.31.090482",
             "NodeName": "hostname:11820",
             "CreateTime": "2022-10-06-18.04.31.090000",
             "UpdateTime": "2022-10-06-18.04.31.164000"
           }
         ]
       }
     ]
   }
   ...
   ```


[^_^]:
    本文使用的所有引用及链接
[syscollection]:manual/Manual/Catalog_Table/SYSCOLLECTIONS.md
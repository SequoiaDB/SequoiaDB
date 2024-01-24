[^_^]:

    数据库快照
    作者：何嘉文
    时间：20190307
    评审意见

    王涛：
    许建辉：
    市场部：20190425


集合空间快照可以列出所有集合空间。用户通过协调节点或非协调节点查看该快照时，返回的结果字段不完全相同。


##标识##

SDB_SNAP_COLLECTIONSPACES

##非协调节点字段信息##

| 字段名          | 类型       | 描述                                         |
| --------------- | ---------- | -------------------------------------------- |
| NodeName        | string     | 集合空间所属节点名，格式为<主机名>:<服务名>  |
| GroupName       | string     | 集合空间所属分区组名                         |
| Name            | string     | 集合空间名                                   |
| UniqueID        | int32      | 集合空间的 UniqueID，在集群上全局唯一        |
| ID              | int32      | 集合空间在节点上的物理 ID                     |
| LogicalID       | int32      | 集合空间在节点上的逻辑 ID                     |
| Collection.Name | string     | 集合空间所包含的集合的名字                   |
| Collection.UniqueID | int64  | 集合空间所包含的集合的 UniqueID               |
| PageSize        | int32      | 集合空间数据页大小，单位为字节               |
| LobPageSize     | int32      | 集合空间大对象数据页大小，单位为字节         |
| MaxCapacitySize | int64      | 集合空间的最大容量上限，单位为字节           |
| MaxDataCapSize  | int64      | 集合空间数据文件最大容量上限，单位为字节     |
| MaxIndexCapSize | int64      | 集合空间索引文件最大容量上限，单位为字节     |
| MaxLobCapSize   | int64      | 集合空间大对象文件最大容量上限，单位为字节<br>v3.6.1 及以上版本中，该字段已更名为 MaxLobCapacity |
| NumCollections  | int32      | 集合数量                                     |
| TotalRecords    | int32      | 集合空间的记录总数                           |
| TotalSize       | int64      | 集合空间的总大小，单位为字节                 |
| FreeSize        | int64      | 集合空间的空闲大小，单位为字节               |
| TotalDataSize   | int64      | 集合空间数据文件总大小，单位为字节           |
| FreeDataSize    | int64      | 集合空间数据文件空闲空间大小，单位为字节     |
| TotalIndexSize  | int64      | 集合空间索引文件总大小，单位为字节           |
| FreeIndexSize   | int64      | 集合空间索引文件空闲空间大小，单位为字节     |
| RecycleDataSize | int64      | 集合空间下所有回收站项目的数据文件总大小，单位为字节（仅在 v3.6.1 及以上版本生效）|
| RecycleIndexSize| int64      | 集合空间下所有回收站项目的索引文件总大小，单位为字节（仅在 v3.6.1 及以上版本生效）|
| RecycleLobSize  | int64      | 集合空间下所有回收站项目的大对象文件总大小，单位为字节（仅在 v3.6.1 及以上版本生效）|
| FreeLobSize    | int64      | 集合空间大对象文件空闲空间大小，单位为字节<br>v3.6.1 及以上版本中，该字段已更名为 FreeLobSpace  |
| MaxLobCapacity  | int64      | 集合空间大对象文件最大容量上限，单位为字节（仅在 v3.6.1 及以上版本生效） |
| LobCapacity     | int64      | 集合空间大对象文件的存储容量，单位为字节（仅在 v3.6.1 及以上版本生效） |
| LobMetaCapacity | int64      | 集合空间大对象元数据文件大小，单位为字节（仅在 v3.6.1 及以上版本生效） |
| TotalLobs       | int64      | 集合空间大对象文件总数（仅在 v3.6.1 及以上版本生效）  |
| TotalLobPages    | int64      | 集合空间大对象文件已使用空间数据页个数（仅在 v3.6.1 及以上版本生效）    |
| TotalUsedLobSpace   | int64      | 集合空间大对象文件已使用的空间大小，单位为字节（仅在 v3.6.1 及以上版本生效） |
| UsedLobSpaceRatio    | double      |  集合空间大对象文件已使用的空间占存储容量的比率（仅在 v3.6.1 及以上版本生效） |
| FreeLobSpace     | int64    | 集合空间大对象文件空闲空间大小，单位为字节（仅在 v3.6.1 及以上版本生效） |
| TotalLobSize     | int64      | 集合空间大对象文件的数据总大小，单位为字节<br>v3.6.1 以下版本该字段为大对象文件的总大小，v3.6.1 及以上版本为大对象文件的数据总大小 |
| TotalValidLobSize | int64     | 集合空间大对象文件有效数据总大小，单位为字节（仅在 v3.6.1 及以上版本生效） |
| LobUsageRate    | double  | 集合空间大对象文件的有效使用率（仅在 v3.6.1 及以上版本生效）<br>使用率越高，空间浪费越少 |
| AvgLobSize      | int64      | 集合空间大对象文件平均大小，单位为字节（仅在 v3.6.1 及以上版本生效） |
| TotalLobGet           | int64     | 客户端获取大对象文件的总次数（仅在 v3.6.1 及以上版本生效） |
| TotalLobPut           | int64     | 客户端上传大对象文件的总次数（仅在 v3.6.1 及以上版本生效） |
| TotalLobDelete        | int64     | 客户端删除大对象文件的总次数（仅在 v3.6.1 及以上版本生效） |
| TotalLobReadSize      | int64     | 客户端读大对象文件的总字节数（仅在 v3.6.1 及以上版本生效） |
| TotalLobWriteSize     | int64     | 客户端写大对象文件的总字节数（仅在 v3.6.1 及以上版本生效） |
| TotalLobRead     | int64     | 服务端中 LOB 分片的读次数（仅在 v3.6.1 及以上版本生效） |
| TotalLobWrite     | int64     | 服务端中 LOB 分片的写次数（仅在 v3.6.1 及以上版本生效） |
| TotalLobTruncate    | int64     | 服务端中 LOB 分片的截断次数（仅在 v3.6.1 及以上版本生效） |
| TotalLobAddressing     | int64     | 服务端中 LOB 分片的寻址总次数（仅在 v3.6.1 及以上版本生效） |
| TotalLobList          | int64     | 客户端列举大对象文件的总次数（仅在 v3.6.1 及以上版本生效） |
| DataCommitLSN   | int64      | 集合空间数据文件最后提交 LSN                 |
| IndexCommitLSN  | int64      | 集合空间索引文件最后提交 LSN                 |
| LobCommitLSN    | int64      | 集合空间大对象文件最后提交 LSN               |
| DataCommitted   | boolean    | 集合空间数据文件当前是否有效提交             |
| IndexCommitted  | boolean    | 集合空间索引文件当前是否有效提交             |
| LobCommitted    | boolean    | 集合空间大对象文件当前是否有效提交           |
| DirtyPage       | int32      | 集合空间大对象文件在开启缓存下脏页数量       |
| Type            | int32      | 集合空间类型，取值如下：<br>0：普通集合空间<br>1：固定（Capped）集合空间 |
| CreateTime | string | 创建集合空间的时间（仅在 v3.6.1 及以上版本生效） |
| UpdateTime | string | 更新集合空间元数据的时间（仅在 v3.6.1 及以上版本生效） |

##协调节点字段信息##

| 字段名          | 类型       | 描述                                         |
| --------------- | ---------- | -------------------------------------------- |
| Name            | string     | 集合空间名                                   |
| UniqueID        | int32      | 集合空间的 UniqueID，在集群上全局唯一        |
| Collection.Name | string     | 集合空间所包含的集合的名字                   |
| Collection.UniqueID | int64 | 集合空间所包含的集合的 UniqueID               |
| PageSize        | int32      | 集合空间数据页大小，单位为字节               |
| LobPageSize     | int32      | 集合空间大对象数据页大小，单位为字节         |
| TotalSize       | int64      | 集合空间的总大小，单位为字节                 |
| FreeSize        | int64      | 集合空间的空闲大小，单位为字节               |
| TotalDataSize   | int64      | 集合空间数据文件总大小，单位为字节           |
| FreeDataSize    | int64      | 集合空间数据文件空闲空间大小，单位为字节     |
| TotalIndexSize  | int64      | 集合空间索引文件总大小，单位为字节           |
| FreeIndexSize   | int64      | 集合空间索引文件空闲空间大小，单位为字节     |
| RecycleDataSize | int64      | 集合空间下所有回收站项目的数据文件总大小，单位为字节（仅在 v3.6.1 及以上版本生效）|
| RecycleIndexSize| int64      | 集合空间下所有回收站项目的索引文件总大小，单位为字节（仅在 v3.6.1 及以上版本生效）|
| RecycleLobSize  | int64      | 集合空间下所有回收站项目的大对象文件总大小，单位为字节（仅在 v3.6.1 及以上版本生效）|
| LobCapacity     | int64      | 集合空间大对象文件的存储容量，单位为字节（仅在 v3.6.1 及以上版本生效） |
| LobMetaCapacity | int64      | 集合空间大对象元数据文件大小，单位为字节（仅在 v3.6.1 及以上版本生效） |
| MaxLobCapacity  | int64      | 集合空间大对象文件最大容量上限，单位为字节（仅在 v3.6.1 及以上版本生效） |
| TotalLobPages    | int64      | 集合空间Lob 已使用空间数据页个数（仅在 v3.6.1 及以上版本生效） |
| TotalLobs       | int64      | 集合空间大对象文件总数（仅在 v3.6.1 及以上版本生效） |
| TotalUsedLobSpace   | int64     | 集合空间大对象文件已使用的空间大小，单位为字节（仅在 v3.6.1 及以上版本生效） |
| UsedLobSpaceRatio   | double    |  集合空间大对象文件已使用的空间占存储容量的比率（仅在 v3.6.1 及以上版本生效） |
| FreeLobSpace     | int64    | 集合空间大对象文件空闲空间大小，单位为字节（仅在 v3.6.1 及以上版本生效） |
| FreeLobSize    | int64      | 集合空间大对象文件空闲空间大小，单位为字节<br>v3.6.1 及以上版本中，该字段已更名为 FreeLobSpace  |
| TotalLobSize     | int64      | 集合空间大对象文件的数据总大小，单位为字节<br>v3.6.1 以下版本该字段为大对象文件的总大小，v3.6.1 及以上版本为大对象文件的数据总大小 |
| TotalValidLobSize | int64     | 集合空间大对象文件有效数据总大小，单位为字节（仅在 v3.6.1 及以上版本生效） |
| LobUsageRate    | double  | 集合空间大对象文件的有效使用率（仅在 v3.6.1 及以上版本生效）<br>使用率越高，空间浪费越少 |
| AvgLobSize     | int64      | 集合空间大对象文件平均大小，单位为字节（仅在 v3.6.1 及以上版本生效） |
| TotalLobGet           | int64     | 客户端获取大对象文件的总次数（仅在 v3.6.1 及以上版本生效） |
| TotalLobPut           | int64     | 客户端上传大对象文件的总次数（仅在 v3.6.1 及以上版本生效） |
| TotalLobDelete        | int64     | 客户端删除大对象文件的总次数（仅在 v3.6.1 及以上版本生效） |
| TotalLobList          | int64     | 客户端列举大对象文件的总次数（仅在 v3.6.1 及以上版本生效） |
| TotalLobReadSize      | int64     | 客户端读大对象文件的总字节数（仅在 v3.6.1 及以上版本生效） |
| TotalLobWriteSize     | int64     | 客户端写大对象文件的总字节数（仅在 v3.6.1 及以上版本生效） |
| TotalLobRead     | int64     | 服务端中 LOB 分片的读次数（仅在 v3.6.1 及以上版本生效） |
| TotalLobWrite     | int64     | 服务端中 LOB 分片的写次数（仅在 v3.6.1 及以上版本生效） |
| TotalLobTruncate    | int64     | 服务端中 LOB 分片的截断次数（仅在 v3.6.1 及以上版本生效） |
| TotalLobAddressing     | int64     | 服务端中 LOB 分片的寻址总次数（仅在 v3.6.1 及以上版本生效） |
| Group           | bson array | 该集合空间所在的复制组名列表                 |
| CreateTime | string | 创建集合空间的时间（仅在 v3.6.1 及以上版本生效） |
| UpdateTime | string | 更新集合空间元数据的时间（仅在 v3.6.1 及以上版本生效） |

##示例##

- 通过非协调节点查看快照

   ```lang-javascript
   > db.snapshot(SDB_SNAP_COLLECTIONSPACES)
   ```

   输出结果如下：

   ```lang-json
   {
     "NodeName": "sdbserver1:11820",
     "GroupName": "group1",
     "Name": "sample",
     "UniqueID": 1,
     "ID": 2,
     "LogicalID": 2,
     "Collection": [
       {
         "Name": "sample.employee",
         "UniqueID": 4294967297
       }
     ],
     "PageSize": 65536,
     "LobPageSize": 262144,
     "MaxCapacitySize": 26388279066624,
     "MaxDataCapSize": 8796093022208,
     "MaxIndexCapSize": 8796093022208,
     "MaxLobCapSize": 8796093022208,
     "NumCollections": 1,
     "TotalRecords": 0,
     "TotalSize": 524517376,
     "FreeSize": 401735659,
     "TotalDataSize": 155254784,
     "FreeDataSize": 133627904,
     "TotalIndexSize": 151060480,
     "FreeIndexSize": 134152171,
     "RecycleDataSize": 0,
     "RecycleIndexSize": 0,
     "RecycleLobSize": 0,
     "FreeLobSize": 133955584,
     "MaxLobCapacity": 8796093022208,
     "LobCapacity": 134217728,
     "LobMetaCapacity": 83984384,
     "TotalLobs": 1,
     "TotalLobPages": 1,
     "TotalUsedLobSpace": 262144,
     "UsedLobSpaceRatio": 0,
     "FreeLobSpace": 133955584,
     "TotalLobSize": 150,
     "TotalValidLobSize": 150,
     "LobUsageRate": 0,
     "AvgLobSize": 150,
     "TotalLobGet": 0,
     "TotalLobPut": 1,
     "TotalLobDelete": 0,
     "TotalLobReadSize": 0,
     "TotalLobWriteSize": 150,
     "TotalLobRead": 0,
     "TotalLobWrite": 1,
     "TotalLobTruncate": 0,
     "TotalLobAddressing": 1,
     "TotalLobList": 0,
     "DataCommitLSN": 80,
     "IndexCommitLSN": 80,
     "LobCommitLSN": 164,
     "DataCommitted": true,
     "IndexCommitted": true,
     "LobCommitted": true,
     "DirtyPage": 0,
     "Type": 0,
     "CreateTime": "2022-10-06-18.04.31.008000",
     "UpdateTime": "2022-10-06-18.05.49.384000"
   }
   ...
   ```

- 通过协调节点查看快照

   ```lang-javascript
   > db.snapshot(SDB_SNAP_COLLECTIONSPACES)
   ```

   输出结果如下：

   ```lang-json
   {
     "Name": "sample",
     "UniqueID": 1,
     "PageSize": 65536,
     "LobPageSize": 262144,
     "TotalSize": 524517376,
     "FreeSize": 401473515,
     "TotalDataSize": 155254784,
     "FreeDataSize": 133627904,
     "TotalIndexSize": 151060480,
     "FreeIndexSize": 134152171,
     "RecycleDataSize": 0,
     "RecycleIndexSize": 0,
     "RecycleLobSize": 0,
     "LobCapacity": 134217728,
     "LobMetaCapacity": 83984384,
     "MaxLobCapacity": 8796093022208,
     "TotalLobPages": 2,
     "TotalLobs": 2,
     "TotalUsedLobSpace": 524288,
     "UsedLobSpaceRatio": 0,
     "FreeLobSpace": 133693440,
     "FreeLobSize": 133693440,
     "TotalLobSize": 300,
     "TotalValidLobSize": 300,
     "LobUsageRate": 0,
     "AvgLobSize": 150,
     "TotalLobGet": 1,
     "TotalLobPut": 2,
     "TotalLobDelete": 0,
     "TotalLobList": 0,
     "TotalLobReadSize": 0,
     "TotalLobWriteSize": 300,
     "TotalLobRead": 1,
     "TotalLobWrite": 2,
     "TotalLobTruncate": 0,
     "TotalLobAddressing": 3,
     "Collection": [
       {
         "Name": "sample.employee",
         "UniqueID": 4294967297
       }
     ],
     "Group": [
       "group1"
     ],
     "CreateTime": "2022-10-06-18.04.31.008000",
     "UpdateTime": "2022-10-06-18.05.49.384000"
   }
   ...
   ```

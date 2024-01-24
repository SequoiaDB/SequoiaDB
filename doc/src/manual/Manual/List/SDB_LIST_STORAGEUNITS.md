[^_^]: 

    存储单元列表
    作者：何嘉文
    时间：20190523
    评审意见

    王涛：
    许建辉：
    市场部：


存储单元列表可以列出所有存储单元信息。

> **Note:**
>  
> 该列表只支持在数据节点上使用。

标识
----

SDB_LIST_STORAGEUNITS

字段信息
----

| 字段名         | 类型   | 描述                        |
| -------------- | ------ | --------------------------- |
| NodeName       | string | 所在的节点名 |
| Name           | string | 集合空间名                  |
| UniqueID       | int32  | 集合空间的 UniqueID，在集群上全局唯一 |
| ID             | int32  | 该集合空间 ID               |
| LogicalID      | string | 集合空间逻辑 ID，为递增顺序 |
| PageSize       | int32  | 集合空间数据页大小          |
| LobPageSize    | int32  | 集合空间大对象页大小        |
| Sequence       | int32  | 序列号，当前版本中为 1      |
| NumCollections | int32  | 集合空间下的集合个数        |
| CollectionHWM  | int32  | 集合高水位，该集合空间中创建过的集合数量（包括被删除的集合） |
| Size           | int64  | 存储单元大小，单位为字节       |
| CreateTime | string | 创建集合空间的时间（仅在 v3.6.1 及以上版本生效） |
| UpdateTime | string | 更新集合空间元数据的时间（仅在 v3.6.1 及以上版本生效） |

示例
----

查看存储单元列表

```lang-javascript
> db.list( SDB_LIST_STORAGEUNITS )
```

输出结果如下：

```lang-json
{
  "NodeName": "r520-8:11890",
  "Name": "sample1",
  "UniqueID": 61,
  "ID": 4095,
  "LogicalID": 0,
  "PageSize": 4096,
  "LobPageSize": 262144,
  "Sequence": 1,
  "NumCollections": 1,
  "CollectionHWM": 1,
  "Size": 172032000,
  "CreateTime": "2022-10-06-18.04.31.008000",
  "UpdateTime": "2022-10-06-18.05.49.384000"
}
{
  "NodeName": "r520-8:11890",
  "Name": "sample2",
  "UniqueID": 62,
  "ID": 4094,
  "LogicalID": 1,
  "PageSize": 4096,
  "LobPageSize": 262144,
  "Sequence": 1,
  "NumCollections": 2,
  "CollectionHWM": 3,
  "Size": 172032000,
  "CreateTime": "2022-10-06-18.04.31.090000",
  "UpdateTime": "2022-10-06-18.04.31.164000"
}
...
```

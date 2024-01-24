
SYSCAT.SYSCOLLECTIONSPACES 集合中包含了该集群中所有的用户集合空间信息，每个用户集合空间保存为一个文档。

每个文档包含以下字段：

|  字段名     |   类型    |    描述                                  |
|-------------|-----------|------------------------------------------|
|  Name       |   string  | 集合空间名                               |
|  Collection |   array   | 集合空间中包含的所有的集合名，每个集合表示为一个嵌套的 JSON 对象，包含 Name 字段与相应的集合名 |
|  PageSize   |   number  | 集合空间的数据页大小，单位为字节   |
| LogPageSize |   number  | 集合空间的大对象 LOB 数据页大小，单位为字节 |
|   Domain    |  string   | 集合空间所属域名（仅指定数据域的集合空间显示）                    |       
|   Type      |   number  | 集合空间类型，0 表示普通集合空间，1 表示固定（Capped）集合空间|
|   UniqueID  |   number  | 集合空间的 UniqueID，在集群上全局唯一 |
| CLUniqueHWM |   number  | 集合 UniqueID 的高水位（仅内部使用）|
| DataSourceID | number   | 集合空间所属数据源 ID（仅使用数据源的集合空间显示）             |
|  Mapping    |  string   | 集合空间在数据源中所映射的集合名称（仅使用数据源的集合空间显示）  |
| CreateTime | string | 创建集合空间的时间（仅在 v3.6.1 及以上版本生效） |
| UpdateTime | string | 更新集合空间元数据的时间（仅在 v3.6.1 及以上版本生效） |  

**示例**

包含一个集合的集合空间信息如下：

```lang-json
{
  "CLUniqueHWM": 4294967297,
  "Collection": [
    {
      "Name": "employee",
      "UniqueID": 4294967297
    }
  ],
  "CreateTime": "2022-10-06-18.04.17.168000",
  "LobPageSize": 262144,
  "Name": "sample",
  "PageSize": 65536,
  "Type": 0,
  "UniqueID": 1,
  "UpdateTime": "2022-10-06-19.51.40.301000",
  "_id": {
    "$oid": "600e85fd62f47c63a09cdf82"
  }
}
```


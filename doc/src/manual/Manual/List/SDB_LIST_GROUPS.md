[^_^]: 

    复制组列表
    作者：何嘉文
    时间：20190522
    评审意见
    
    王涛：
    许建辉：
    市场部：



数据分区列表可以列出集群中所有复制组信息。

标识
----

SDB_LIST_GROUPS

字段信息
----

| 字段名             | 类型       | 描述                                                |
| ------------------ | ---------  | --------------------------------------------------- |
| Group.dbpath       | string     | 复制组中节点的数据文件存放路径                                    |
| Group.Status       | int32      | 复制组中节点的状态，取值如下：<br>1：已激活<br>0：未激活 |
| Group.HostName     | string     | 复制组中节点的主机名                           |
| Group.Service.Type | int32   | 复制组中节点的服务类型，取值如下：<br> 0：直连服务，对应数据库参数 svcname <br> 1：复制服务，对应数据库参数 replname <br> 2：分区服务，对应数据库参数 shardname<br> 3：编目服务，对应数据库参数 catalogname |
| Group.Service.Name | string | 复制组中节点的服务名，服务名可以为端口号，或 services 文件中的服务名 |
| Group.NodeID       | int32  | 复制组中节点的 ID              |
| Group.Location     | string | 复制组中节点的位置信息（仅在已设置位置集的节点显示）        |
| GroupID            | int32      | 复制组 ID                                           |
| GroupName          | string     | 复制组名称                                            |
| Locations.Location | string     | 复制组中节点的位置信息（仅在已设置位置集的节点显示）  |
| Locations.LocationID | int32    | 复制组中节点的位置信息 ID（仅在已设置位置集的节点显示） |
| Locations.PrimaryNode | int32    | 复制组中位置集的主节点 ID（仅在已设置位置集的节点显示） |
| PrimaryNode        | int32      | 主节点 ID                                           |
| Role               | int32      | 复制组角色，取值如下：<br>0：数据节点<br>1：协调节点<br>2：编目节点 |
| Status             | string     | 复制组状态，取值如下：<br>1：已激活<br>0：未激活<br>不存在：未激活分区组            |
| Version            | int32      | 复制组版本号（内部使用） |
| ActiveLocation     | string     | 复制组中的 ActiveLocation |


示例
----

查看数据分区列表

```lang-javascript
> db.list( SDB_LIST_GROUPS )
```

输出结果如下：

```lang-json
{
  "ActiveLocation": "GuangZhou",
  "Group":[
    {
      "HostName": "hostname1",
      "Status": 1,
      "dbpath": "/data/disk1/sequoiadb/database/catalog/11860/",
      "Service": [
        {
          "Type": 0,
          "Name": "11800"
        },
        {
          "Type": 1,
          "Name": "11801"
        },
        {
          "Type": 2,
          "Name": "11802"
        },
        {
          "Type": 3,
          "Name": "11803"
        }
      ],
      "NodeID": 1,
      "Location": "GuangZhou"
    }
  ],
  "GroupID": 1,
  "GroupName": "SYSCatalogGroup",
  "Locations": [
    {
      "Location": "GuangZhou",
      "LocationID": 1,
      "PrimaryNode": 1
    }
  ],
  "PrimaryNode": 1,
  "Role": 2,
  "Status": 1,
  "Version": 1,
  "_id": {
    "$oid": "51710981d8cb8fbc163d6350"
  }
}
...
```

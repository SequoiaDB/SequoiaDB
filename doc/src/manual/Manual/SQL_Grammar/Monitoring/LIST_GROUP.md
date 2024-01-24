
复制组列表可以列出当前集群中的所有分区信息。

##标识##

$LIST_GROUP

##字段信息##

| 字段名             | 类型   | 描述                           |
| ------------------ | ------ | ------------------------------ |
| Group.dbpath       | string | 复制组中节点的数据文件存放路径 |
| Group.Status       | int32   | 复制组中节点的状态             |
| Group.HostName     | string | 复制组中节点的主机名           |
| Group.Service.Type | int32   | 复制组中节点的服务类型，取值如下：<br>0：直连服务，对应数据库参数 svcname <br> 1：复制服务，对应数据库参数 replname <br>2：分区服务，对应数据库参数 shardname<br>3：编目服务，对应数据库参数 catalogname |
| Group.Service.Name | string | 复制组中节点的服务名，服务名可以为端口号，或 services 文件中的服务名 |
| Group.NodeID       | int32   | 复制组中节点的 ID              |
| Group.Location     | string  | 复制组中节点的位置信息        |
| GroupID            | int32    | 复制组 ID                      |
| GroupName          | string | 复制组名称                     |
| Locations.Location | string     | 复制组中节点的位置信息  |
| Locations.LocationID | int32    | 复制组中节点的位置信息 ID |
| Locations.PrimaryNode | int32    | 复制组中位置集的主节点 ID |
| PrimaryNode        | int32     | 主节点 ID                      |
| Role               | int32     | 复制组角色，取值如下：<br> 0：数据节点<br>2：编目节点 |
| Status             | string | 复制组状态，取值如下：<br> 1：已激活分区组<br> 0：未激活分区组<br> 不存在：未激活分区组 |
| Version            | int32      | 复制组版本号（内部使用） |
| ActiveLocation     | string     | 复制组中的 ActiveLocation |

##示例##

查看复制组列表

```lang-javascript
> db.exec( "select * from $LIST_GROUP" )
```

输出结果如下：

```lang-json
{
  "ActiveLocation": "GuangZhou",
  "Group": [
    {
      "dbpath": "/opt/test/30000",
      "HostName": "hostname",
      "Service": [
        {
          "Type": 0,
          "Name": "30000"
        },
        {
          "Type": 1,
          "Name": "30001"
        },
        {
          "Type": 2,
          "Name": "30002"
        },
        {
          "Type": 3,
          "Name": "30003"
        }
      ],
      "NodeID": 1,
      "Location": "GuangZhou",
      "Status": 1
    },
    {
      "HostName": "hostname",
      "dbpath": "/opt/test/30020",
      "Service": [
        {
          "Type": 0,
          "Name": "30020"
        },
        {
          "Type": 1,
          "Name": "30021"
        },
        {
          "Type": 2,
          "Name": "30022"
        },
        {
          "Type": 3,
          "Name": "30023"
        }
      ],
      "NodeID": 3,
      "Location": "ShenZhen",
      "Status": 1
    },
    {
      "HostName": "hostname",
      "Status": 1,
      "dbpath": "/opt/test/30010/",
      "Service": [
        {
          "Type": 0,
          "Name": "30010"
        },
        {
          "Type": 1,
          "Name": "30011"
        },
        {
          "Type": 2,
          "Name": "30012"
        },
        {
          "Type": 3,
          "Name": "30013"
        }
      ],
      "NodeID": 2
    }
  ],
  "GroupID": 1,
  "GroupName": "SYSCatalogGroup",
  "Locations": [
    {
      "Location": "GuangZhou",
      "LocationID": 1,
      "PrimaryNode": 1
    },
    {
      "Location": "ShenZhen",
      "LocationID": 2
      "PrimaryNode": 3
    }
  ],
  "PrimaryNode": 1,
  "Role": 2,
  "SecretID": 1831753872,
  "Status": 1,
  "Version": 3,
  "_id": {
    "$oid": "5cf0855907c2e1754b77b42f"
  }
}
...
```

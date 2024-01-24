[^_^]:
     SYSCAT.SYSGROUPMODES 集合

SYSCAT.SYSGROUPMODES 集合中包含了该集群中所有复制组的运行模式，每个复制组保存为一个文档。

每个文档包含以下字段：

| 字段名          | 类型       | 描述 |
| --------------- | ---------- | ---- |
| GroupID         | number     | 复制组 ID |
| GroupName       | string     | 复制组名 |
| GroupMode       | string     | 复制组的运行模式 |
| Properties.NodeID | number    | 运行模式生效的节点 ID |
| Properties.NodeName | string    | 运行模式生效的节点名 |
| Properties.LocationID | number | 运行模式生效的位置集 ID |
| Properties.Location | string | 运行模式生效的位置集 |
| Properties.MinKeepTime | string | 运行模式的最低运行窗口时间 |
| Properties.MaxKeepTime | string | 运行模式的最高运行窗口时间 |
| Properties.UpdateTime | string | 运行模式的更新时间 |

##示例##

复制组的运行模式为 Critical 时，信息如下：

```lang-json
{
  "_id": {
    "$oid": "6433d568af7a35b253a4c5d8"
  },
  "GroupID": 1001,
  "GroupName": "group1",
  "GroupMode": "critical",
  "Properties": [
    {
      "NodeID": 1001,
      "NodeName": "sdbserver:11820",
      "MinKeepTime": "2023-04-10-17.27.45.142311",
      "MaxKeepTime": "2023-04-10-17.32.45.142311",
      "UpdateTime": "2023-04-10-17.22.45.142311"
    }
  ]
}
{
  "_id": {
    "$oid": "642aa060e8e128392794f85a"
  },
  "GroupID": 1002,
  "GroupName": "group2",
  "GroupMode": "critical",
  "Properties": [
    {
      "Location": "GZ.nansha",
      "MinKeepTime": "2023-04-03-17.51.05.289544",
      "MaxKeepTime": "2023-04-03-17.56.05.289544",
      "UpdateTime": "2023-04-03-17.46.05.289544"
    }
  ]
}
```

复制组的运行模式为 Maintenance 时，信息如下：

```lang-json
{
  "_id": {
    "$oid": "6433d568af7a35b253a4c5d8"
  },
  "GroupID": 1001,
  "GroupName": "group1",
  "GroupMode": "maintenance",
  "Properties": [
    {
      "NodeID": 1001,
      "NodeName": "sdbserver:11820",
      "MinKeepTime": "2023-04-10-17.27.45.142311",
      "MaxKeepTime": "2023-04-10-17.32.45.142311",
      "UpdateTime": "2023-04-10-17.22.45.142311"
    },
    {
      "NodeID": 1002,
      "NodeName": "sdbserver:11830",
      "MinKeepTime": "2023-04-10-17.27.45.142311",
      "MaxKeepTime": "2023-04-10-17.32.45.142311",
      "UpdateTime": "2023-04-10-17.22.45.142311"
    }
  ]
}
```


>**Note:**
>
> - 在 Critical 模式中，Properties 字段的数组仅包含一个元素。
> - 在 Critical 模式中，Properties 数组元素的 NodeID/NodeName 和 LocationID/Location 字段仅包含其中一种。
> - 在 Maintenance 模式中，Properties 字段的数组可包含多个元素。
> - 在 Maintenance 模式中，Properties 数组元素仅包含 NodeID/NodeName 字段。

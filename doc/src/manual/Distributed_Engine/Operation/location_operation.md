[^_^]:
    位置集操作

位置集是复制组中拥有相同位置信息的节点集合，用户可通过设置、修改、删除节点的位置信息，实现位置集内节点的管理。下述以名为"group1"的复制组为例，介绍 [setLocation][setLocation] 管理节点位置信息的相关操作。

##设置节点位置信息##

将节点 11820 的位置信息设置为"GuangDong.guangzhou"

```lang-javascript
> var node = db.getRG("group1").getNode("sdbserver", 11820)
> node.setLocation("GuangDong.guangzhou")
```

##修改节点位置信息##

将节点 11820 的位置信息修改为"GuangDong.shenzhen"

```lang-javascript
> var node = db.getRG("group1").getNode("sdbserver", 11820)
> node.setLocation("GuangDong.shenzhen")
```

##删除节点位置信息##

删除节点 11820 的位置信息，删除后该节点将不属于任何一个位置集

```lang-javascript
> var node = db.getRG("group1").getNode("sdbserver", 11820)
> node.setLocation("")
```

##查看位置集##

通过[分区组列表][SDB_LIST_GROUPS]查看复制组下的位置集信息

```lang-javascript
> db.list(SDB_LIST_GROUPS, {GroupName: "group1"}, {Locations: ""})
```

输出结果如下：

```lang-json
{
  "Locations": [
    {
      "Location": "GuangDong.guangzhou",
      "LocationID": 1,
      "PrimaryNode": 1000
    },
    {
      "Location": "GuangDong.shenzhen",
      "LocationID": 2,
      "PrimaryNode": 1001
    }
  ]
...
}
```

##参考##

更多操作可参考

| 操作 | 说明 |
| ---- | ---- |
| [domain.setLocation][domain_setLocation] | 批量修改域中节点的位置信息 |

[^_^]:
     本文使用的所有引用及链接
[setLocation]:manual/Manual/Sequoiadb_Command/SdbNode/setLocation.md
[SDB_LIST_GROUPS]:manual/Manual/List/SDB_LIST_GROUPS.md
[domain_setLocation]:manual/Manual/Sequoiadb_Command/SdbDomain/setLocation.md
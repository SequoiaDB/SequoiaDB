##名称##

getDetailObj - 获取复制组的信息

##语法##

**rg.getDetailObj()**

##类别##

SdbReplicaGroup

##描述##

获取当前复制组的详细信息，复制组的详细信息可参考[数据分区列表][LIST_GROUPS]章节。

##参数##

无

##返回值##

函数执行成功时，返回当前复制组详细信息，其类型为 BSONObj。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.2.7 及以上版本

v3.4.2 及以上版本

##示例##

获取 group1 复制组的详细信息，该复制组存在 1 个节点。

```lang-javascript
> var rg = db.getRG("group1")
> rg.getDetailObj()
```

结果如下：

```lang-text
{
  "ActiveLocation": "GuangZhou",
  "Group": [
    {
      "HostName": "localhost",
      "Status": 1,
      "dbpath": "/opt/sequoiadb/database/data/11830/",
      "Service": [
        {
          "Type": 0,
          "Name": "11830"
        },
        {
          "Type": 1,
          "Name": "11831"
        },
        {
          "Type": 2,
          "Name": "11832"
        }
      ],
      "NodeID": 1002,
      "Location": "GuangZhou"
    }
  ],
  "GroupID": 1001,
  "GroupName": "group1",
  "Locations": [
    {
      "Location": "GuangZhou",
      "LocationID": 1,
      "PrimaryNode": 1002
    }
  ],
  "PrimaryNode": 1004,
  "Role": 0,
  "Status": 1,
  "Version": 7,
  "_id": {
    "$oid": "580043577e70618777a2cf39"
  }
}
```


[^_^]:
     本文使用的所有引用及链接
[LIST_GROUPS]:manual/Manual/List/SDB_LIST_GROUPS.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[faq]:manual/FAQ/faq_sdb.md

##名称##

listReplicaGroups - 枚举复制组信息

##语法##

**db.listReplicaGroups()**

##类别##

Sdb

##描述##

该函数用于枚举复制组信息。

##参数##

无

##返回值##

函数执行成功时，将返回游标对象。通过游标对象获取的结果字段说明可查看 [$LIST_GROUP][LIST_GROUP]

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][error_guide]。

##版本##

v2.0 及以上版本

##示例##

* 返回所有复制组信息

    ```lang-javascript
    > db.listReplicaGroups()
    {
    "ActiveLocation": "GuangZhou",
    "Group": 
    [
      {
        "dbpath": "/opt/sequoiadb/data/11800",
        "HostName": "vmsvr2-suse-x64",
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
        "NodeID": 1000,
        "Location": "GuangZhou"
      },
      {
        "dbpath": "/opt/sequoiadb/data/11850",
        "HostName": "vmsvr2-suse-x64",
        "Service": [
          {
            "Type": 0,
            "Name": "11850"
          },
          {
            "Type": 1,
            "Name": "11851"
          },
          {
            "Type": 2,
            "Name": "11852"
          },
          {
            "Type": 3,
            "Name": "11853"
          }
        ],
        "NodeID": 1001
      }
    ],
    "GroupID": 1001,
    "GroupName": "group",
    "Locations": [
      {
        "Location": "GuangZhou",
        "LocationID": 1,
        "PrimaryNode": 1000
      }
    ],
    "PrimaryNode": 1001,
    "Role": 0,
    "Status": 1,
    "Version": 5,
    "_id": {
      "$oid": "517b2fc33d7e6f820fc0eb57"
      }
    }
    ```

    这个复制组有两个节点：11800和11850，其中11850为主节点。复制组详细信息请见[复制组列表][LIST_GROUPS]


[^_^]:
     本文使用的所有引用及链接
[LIST_GROUP]:manual/Manual/SQL_Grammar/Monitoring/LIST_GROUP.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md
[LIST_GROUPS]:manual/Manual/List/SDB_LIST_GROUPS.md
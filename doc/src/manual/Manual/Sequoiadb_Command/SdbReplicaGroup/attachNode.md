##名称##

attachNode - 将不属于任何复制组的节点加入当前复制组

##语法##

**rg.attachNode( \<host\>, \<service\>, \<options\> )**

##类别##

SdbReplicaGroup

##描述##

将一个已经创建完成但不属于任何复制组的节点加入到当前复制组。可以搭配 [rg.detachNode()](manual/Manual/Sequoiadb_Command/SdbReplicaGroup/detachNode.md) 使用。目前可以支持加入到数据组或者编目组。

##参数##

| 参数名  | 参数类型  | 描述                         | 是否必填 |
| ------- | --------- | -----------------------------| -------- |
| host    | string    | 节点的主机名或者主机 IP。    | 是 |
| service | string    | 节点服务名或者端口。         | 是 |
| options | Json 对象 | 详见options选项说明。| 是 |

options 选项：

| 参数名   | 参数类型 | 描述                        | 默认值 |
| -------- | -------- | --------------------------- | ------ |
| KeepData | bool     | 是否保留新加节点原有的数据。| 无默认值，需用户显式指定。  |

> **Note:**  

> 1. 参数 options 中的 KeepData 字段为必填项，需用户显式指定。由于该选项会决定新节点数据是否继续被保留，用户应该谨慎考虑。
> 2. 如果新加的节点原本不属于当前组，建议用户将 KeepData 设置为 false。否则，一旦发生主备切换及全量同步，当前组原有节点的数据将有可能被新加节点的数据覆盖。
> 3. 节点配置文件中角色(role)指定为编目(catalog)的节点只能加入编目组中；角色指定为数据(data)的节点只能加入到数据组中。

##返回值##

无返回值，出错抛异常，并输出错误信息。可以通过 [getLastErrMsg](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) 获取错误信息，或通过 [getLastError](manual/Manual/Sequoiadb_Command/Global/getLastError.md) 获取错误码。
关于错误处理可以参考 [常见错误处理指南](manual/FAQ/faq_sdb.md) 。

##错误##

错误信息记录在节点诊断日志（diaglog）中，可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

| 错误码 | 可能的原因 | 解决方法 |
| -------- | ---------------------- | ------------------------------- |
| -15      | 网络错误               | 1. 检查 sdbcm 状态是否正常，如果状态异常，可以尝试重启；<br> 2. 检查填写的 host 是否正确。 |
| -146     | 节点不存在             | 检查节点是否存在。 |
| -157     | 节点已存在于其他复制组 | 检查节点是否已加入到当前或其他复制组，如果已属于任何复制组将不支持该操作。注意：编目节点不能加入到数据组中，数据节点也不能加入到编目组中。 |

##版本##

v2.0 及以上版本

##示例##

将一个节点从 group1 中分离，加入到 group2 中，方法如下：

attachNode 前的节点信息：

```lang-javascript
> db.listReplicaGroups()
{
  "Group": [
    {
      "HostName": "hostname1",
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
      "NodeID": 1007
    }
    ......
  ],
  "GroupID": 1002,
  "GroupName": "group1",
  "PrimaryNode": 1002,
  "Role": 0,
  "Status": 1,
  "Version": 7,
  "_id": {
    "$oid": "580adfd531ae32109e38ca29"
  }
}
......
{
  "Group": [
    {
      "HostName": "hostname2",
      "Status": 1,
      "dbpath": "/opt/sequoiadb/database/data/11840/",
      "Service": [
        {
          "Type": 0,
          "Name": "11840"
        },
        {
          "Type": 1,
          "Name": "11841"
        },
        {
          "Type": 2,
          "Name": "11842"
        }
      ],
      "NodeID": 1000
    }
    ......
  ],
  "GroupID": 1000,
  "GroupName": "group2",
  "PrimaryNode": 1000,
  "Role": 0,
  "Status": 1,
  "Version": 7,
  "_id": {
    "$oid": "580adfd531ae32109e38ca27"
  }
}
```

将“hostname1:11830” 节点从 group1 复制组中分离：

```lang-javascript
> db.getRG('group1').detachNode('hostname1', '11830', { KeepData: true } )
```

将“hostname1:11830” 节点加入到 group2 复制组中，由于节点原本不属于 group2, 此处将其原本的数据清空：

```lang-javascript
> db.getRG('group2').attachNode('hostname1', '11830', { KeepData: false } )
```

查看 attachNode 后的节点信息，group1 复制组中已不存在“hostname1:11830” 节点，group2 复制组存在“hostname1:11830” 节点：

```lang-javascript
> db.listReplicaGroups()
    {
      "HostName": "hostname3",
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
      "NodeID": 1002
    }
  ],
  "GroupID": 1002,
  "GroupName": "group1",
  "PrimaryNode": 1002,
  "Role": 0,
  "Status": 1,
  "Version": 7,
  "_id": {
    "$oid": "580adfd531ae32109e38ca29"
  }
}
......
{
  "Group": [
    {
      "HostName": "hostname1",
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
      "NodeID": 1010
    },
    {
      "HostName": "hostname2",
      "Status": 1,
      "dbpath": "/opt/sequoiadb/database/data/11840/",
      "Service": [
        {
          "Type": 0,
          "Name": "11840"
        },
        {
          "Type": 1,
          "Name": "11841"
        },
        {
          "Type": 2,
          "Name": "11842"
        }
      ],
      "NodeID": 1000
    },
    ......
  ],
  "GroupID": 1000,
  "GroupName": "group2",
  "PrimaryNode": 1000,
  "Role": 0,
  "Status": 1,
  "Version": 7,
  "_id": {
    "$oid": "580adfd531ae32109e38ca27"
  }
}
```

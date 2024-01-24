##名称##

reelectLocation - 重新选举位置集中的主节点

##语法##

**rg.reelectLocation(\<location\>, [options])**

##类别##

SdbReplicaGroup

##描述##

该函数用于在当前复制组中，对指定位置集的主节点进行重选举。

##参数##

- location（ *string，必填* ）

    位置信息

- options（ *object，选填* ）

    通过参数 options 可以设置主节点的匹配条件：

    - Seconds（ *number* ）：选举的超时时间，默认值为 30，单位为秒

        格式：`Seconds: 50`

    - NodeID（ *number* ）：期望当选主节点的节点 ID

        格式：`NodeID: 1000`

    - HostName（ *string* ）：期望当选主节点的主机名

        如果指定了参数 NodeID，该参数不生效。

        格式：`HostName: "hostname"`

    - ServiceName（ *string* ）：期望当选主节点的服务名

        如果指定了参数 NodeID，该参数不生效。

        格式：`ServiceName: "11820"`

>**Note:**
>
> - 当匹配到多个节点时，将在所匹配到的节点中随机选择主节点。
> - 如果未指定参数 NodeID、HostName 和 ServiceName，系统将依据[选举机制][Replication]自动匹配节点。
> - 同时指定参数 HostName 和 ServiceName，在参数生效的情况下，将匹配同时满足两个条件的节点。

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`reelectLocation()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| ------ | -------- | -------------- | -------- |
| -6 | SDB_INVALIDARG | 参数类型错误 | 检查参数类型是否正确 |
| -259 | SDB_OUT_OF_BOUND | 未指定必填参数 | 检查是否缺少必填参数 |
| -334 | SDB_OPERATION_CONFLICT | 参数 location 的值为主位置集 | 目前不支持在主位置集中执行重选举操作，需检查参数 location 的值是否正确|
| -395 | SDB_CLS_NOT_LOCATION_PRIMARY | 位置集缺少主节点 | 检查当前位置集的节点状态，保证可用节点数超过位置集总节点数的一半 | 

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.6.1 及以上版本

##示例##

1. 对复制组 group1 下的位置集 GuangZhou 执行重选举操作，将 NodeID 为 1000 的节点设置为主节点

    ```lang-javascript
    > var rg = db.getRG("group1")
    > rg.reelectLocation("GuangZhou", {NodeID: 1000})
    ```

2. 查看当前位置集对应的主节点 ID

    ```lang-javascript
    > db.list(SDB_LIST_GROUPS, {"GroupName": "group1"}, {"Locations.Location": null, "Locations.PrimaryNode": null})
    {
      "Locations": [
        {
          "Location": "GuangZhou",
          "PrimaryNode": 1000
        }
      ]
    }
    ```

[^_^]:
     本文使用的所有引用及链接
[listReplicaGroups]:manual/Manual/Sequoiadb_Command/Sdb/listReplicaGroups.md
[Replication]:manual/Distributed_Engine/Architecture/Replication/election.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
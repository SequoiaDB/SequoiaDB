##名称##

reelect - 在当前复制组中重新选举

##语法##

**rg.reelect( [options] )**

##类别##

SdbReplicaGroup

##描述##

在当前复制组中重新选举。

##参数##

| 参数名  | 参数类型  | 描述       | 是否必填 |
| ------- | ----------| ---------- | -------- |
| options | Json 对象 | 可选项，详见如下option选项说明。 | 否       |

options 参数详细说明如下：

| 参数名   | 参数类型 | 描述                        | 默认值 |
| ---------| -------- | --------------------------- | ------ |
| Seconds  | int      | 重新选举需要在多少秒内完成。| 30     |
| NodeID   | int      | 期望当选主节点的节点ID      |        |
| HostName | string   | 期望当选主节点的主机名      |        |
| ServiceName | string | 期望当选主节点的服务名     |        |

> **Note:**  
>
> 1. 返回超时错误代表在规定时间内重选没有完成。如果返回成功，需等待若干秒，待编目信息异步更新完成后，再通过 [db.listReplicaGroups()][istReplicaGroups] 观察最终结果。  

> 2. 只有复制组中存在主节点时才可以进行重新选举。  

> 3. 当使用NodeID时，则HostName、ServiceName不生效。  

> 4. 当没有指定具体的 NodeID、ServiceName 的情况下，如果有多个节点都可当选主节点时，此时选举的匹配规则为：节点的 LSN 号（日志序号） > 节点权重 > 节点 ID，优先选取 LSN 号最大的节点，若 LSN 号一致，则选取权重值大的节点，若节点权重一致，则选取节点 ID 最大的节点为主节点。节点的权重设置可以参考[数据库配置][cluster_config]。   

> 5. 复制组中的可用节点（存活节点）至少需要占总节点数半数以上，才能进行选举。  

> 6. 选举的具体描述可以参考[选举机制][Replication]。  


##返回值##

无返回值，出错抛异常，并输出错误信息。

##错误##

`reelect()`函数常见异常如下：

| 错误码 | 错误类型 | 描述 | 解决方法 |
| ------ | --- | ------------ | ----------- |
| -104 | SDB_CLS_NOT_PRIMARY| 分区组不存在主节点 | 检查当前分区组是否存在 "IsPrimary" 为 "true" 的节点。若当前分区组存在节点未启，请启动节点。|

如果出错则抛异常，并输出错误信息，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息，通过 [getLastError()][getLastError] 获取错误码。常见错误处理可以参考[常见错误处理指南][faq]。

[错误码][error_code]

##版本##

v2.0 及以上版本

##示例##

在 group1 中进行重新选举，超时时间为60s。

```lang-javascript
> var rg = db.getRG("group1")
> rg.reelect({Seconds:60})
```


[^_^]:
    本文使用的所有引用和链接
[istReplicaGroups]:manual/Manual/Sequoiadb_Command/Sdb/listReplicaGroups.md
[cluster_config]:manual/Distributed_Engine/Maintainance/Database_Configuration/configuration_parameters.md
[Replication]:manual/Distributed_Engine/Architecture/Replication/election.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
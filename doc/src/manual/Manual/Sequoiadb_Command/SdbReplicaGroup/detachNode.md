##名称##

detachNode - 分离当前复制组内的一个节点

##语法##

**rg.detachNode( \<host\>, \<service\>, \<options\> )**

##类别##

SdbReplicaGroup

##描述##

分离当前复制组内的一个节点，其配置信息不会被删除。搭配 [rg.attachNode()](manual/Manual/Sequoiadb_Command/SdbReplicaGroup/attachNode.md)使用。目前可以支持从数据组或者编目组中分离节点。

##参数##

| 参数名  |  参数类型  |  描述                   |  是否必填 |
| ------- | ---------- | ----------------------- | --------- |
| host    |  string    | 节点的主机名或者主机 IP。  | 是 |
| service |  string    | 节点服务名或者端口。       | 是 |
| options |  Json 对象 | 详见如下options选项说明。 | 是 |

options 选项：

| 参数名  |  参数类型  |  描述                        |  默认值 |
| ------- | ---------- | ---------------------------- | ------- |
| KeepData  | bool     | 是否保留目标节点原有的数据。 |  无默认值，需用户显式指定。  |
| Enforced  | bool     | 是否强制分离节点             |  false  |

> **Note:**
>
> 1. 参数 options 中的 KeepData 字段为必填项，需用户显式指定。由于该选项会决定被detach的节点的数据是否继续被保留，用户应该谨慎考虑。
> 2. 主节点或复制组内只有一个节点时，该节点将不能被 detach，如需强制删除请使用 Enforced 参数。
> 3. 分离后的节点将不再受集群管理，请尽快加入到其他复制组中。

##返回值##

无返回值，出错抛异常，并输出错误信息。可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息，通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

##错误##

错误信息记录在节点诊断日志（diaglog）中，可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

| 错误码 | 可能的原因           | 解决方法 |
| ------ | -------------------- | ---------------------------------------------------- |
| -15    | 网络错误             | 1. 检查 sdbcm 状态是否正常；<br>2. 检查 host 是否正确，网络是否能正常通信。  |
| -155   | 节点不属于当前复制组 | 检查节点是否属于当前复制组。 |
| -204   | 尝试分离主节点，<br>或者组内最后一个节点 | 如果需要强制删除，可以加入 { Enforced: true } 选项。 |

##版本##

v2.0 及以上版本

##示例##

见 [rg.attachNode()](manual/Manual/Sequoiadb_Command/SdbReplicaGroup/attachNode.md) 中示例说明。


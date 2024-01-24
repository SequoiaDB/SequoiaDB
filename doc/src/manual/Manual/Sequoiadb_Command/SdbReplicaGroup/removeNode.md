##名称##

removeNode - 删除当前复制组中的指定节点

##语法##

**rg.removeNode( \<host\>, \<service\>, [options] )**

##类别##

SdbReplicaGroup

##描述##

删除当前复制组中的指定节点。

##参数##

| 参数名  | 参数类型   | 描述           | 是否必填 |
|---------|------------|----------------|----------|
| host    | string     | 节点主机名。   | 是       |
| service | int/string | 节点端口号。   | 是       |
| options | Json 对象 | 可选项，详见如下options选项说明。 | 否 |

options 选项：

| 参数名  |  参数类型  |  描述                        |  默认值 |
| ------- | ---------- | ---------------------------- | ------- |
| Enforced | bool      | 是否强制删除节点。           |  false  |

##返回值##

无返回值，出错抛异常，并输出错误信息。可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息，通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

##错误##

| 错误码 | 可能的原因 | 解决方法 |
| ------ | ------ | ------ |
| -204   | 尝试删除主节点，<br>或者组内最后一个节点 | 如果需要强制删除空的主节点，可以加入 { Enforced: true } 选项 |
| -206   | 尝试删除主编目节点 | 只能删除备编目节点 |
| -79    | 删除节点主机上的CM进程不存在，<br>或者主机宕机 | 如果需要强制删除，可以加入 { Enforced: true } 选项 |

[错误码](manual/Manual/Sequoiadb_error_code.md)

##版本##

v2.0 及以上版本

##示例##

删除 group1 复制组中节点

```lang-javascript
> var rg = db.getRG( "group1" )
> rg.removeNode( "vmsvr2-suse-x64", 11800 )
```

强制删除 group1 复制组中的节点

```lang-javascript
> var rg = db.getRG("group1")
> rg.removeNode( "vmsvr2-suse-x64", 11800, { Enforced: true } )
```

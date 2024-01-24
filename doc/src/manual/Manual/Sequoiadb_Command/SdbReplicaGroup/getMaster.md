##名称##

getMaster - 获取当前复制组的主节点

##语法##

**rg.getMaster()**

##类别##

SdbReplicaGroup

##描述##

该函数用于获取当前复制组的主节点。

##参数##

无

##返回值##

函数执行成功时，将返回一个 SdbNode 类型的对象。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.0 及以上版本

##示例##

获取 group1 复制组的主节点，可以通过该节点进行相关的节点级操作

```lang-javascript
> var rg = db.getRG("group1")
> var node = rg.getMaster()
> println(node)
hostname1:11830
> println(node.constructor.name)
SdbNode
> node.help()

   --Instance methods for class "SdbNode":
   connect()                  - Connect the database to the current node.
   getHostName()              - Return the hostname of a node.
   getNodeDetail()            - Return the information of the current node.
   getServiceName()           - Return the server name of a node.
   start()                    - Start the current node.
   stop()                     - Stop the current node.
```

[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
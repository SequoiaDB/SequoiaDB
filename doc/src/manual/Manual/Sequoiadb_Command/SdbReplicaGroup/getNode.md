##名称##

getNode - 获取当前复制组的指定节点

##语法##

**rg.getNode(\<nodename\>|\<hostname\>, \<servicename\>)**

##类别##

SdbReplicaGroup

##描述##

该函数用于获取当前复制组的指定节点。

##参数##

| 参数名 | 类型 | 描述 | 是否必填 |
| ------ | -------- | ------ | -------- |
| nodename | string | 节点名称| nodename 与 hostname 二选一 |
| hostname | string | 主机名  | hostname 与 nodename 二选一 |
| servicename | string | 服务器名称 | 是 |

> **Note:** 
> 
> rg.getNode() 方法定义了两个参数，第一个参数可是节点名称也可以是主机名，第二个参数为服务器名称。两个参数的类型都是字符串型，且必填。  
> 格式: ("\<节点名称\>|\<主机名\>", "\<服务器名称\>")

##返回值##

函数执行成功时，将返回一个 SdbNode 类型的对象。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.0 及以上版本

##示例##

指定主机名和服务器名，获取该指定节点

```lang-javascript
> var rg = db.getRG("group1")
> rg.getNode("hostname1", "11830")
hostname1:11830
```

[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
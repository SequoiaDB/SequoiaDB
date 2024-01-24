##名称##

startRG - 启动复制组

##语法##

**db.startRG(\<name1\>, [name2], ...)**

##类别##

Sdb

##描述##

该函数用于启动指定的复制组。复制组启动后才能在复制组上创建节点。该方法等价于[rg.start()][start]。

##参数##

| 参数名 | 类型    | 描述 			| 是否必填 	 |
| ------ | ------ 	| ------ 		| ------	 |
| name1, name2... 	 | string 	| 复制组的名称 	| 是 		 |

> **Note:**
>
> 若指定的复制组不存在，将抛异常。

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.0 及以上版本

##示例##

启动复制组的命令如下：

```lang-javascript
> db.startRG("group1")
> db.startRG("group2", "group3", "group4")
```

[^_^]:
     本文使用的所有引用及链接
[start]:manual/Manual/Sequoiadb_Command/SdbReplicaGroup/start.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
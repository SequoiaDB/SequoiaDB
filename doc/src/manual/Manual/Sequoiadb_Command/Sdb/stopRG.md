##名称##

stopRG - 停止复制组

##语法##

**db.stopRG(\<name1\>, [name2], ...)**

##类别##

Sdb

##描述##

该函数用于停止指定的复制组。停止后将不能执行创建节点等相关操作。该方法等价于[rg.stop()][stop]。

##参数##

| 参数名 | 类型     | 描述 			| 是否必填 	 |
| ------ | ------ 	| ------ 		| ------	 |
| name1, name2...	 | string 	| 复制组的名称 	| 是 		 |

> **Note:**
>
> - 若指定的复制组不存在，将抛异常。
> - 若不指定任何复制组，该操作为空操作。

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.8 及以上版本

##示例##

停止复制组的命令如下：

```lang-javascript
> db.stopRG("group1")
> db.stopRG("group2", "group3", "group4")
```

[^_^]:
     本文使用的所有引用及链接
[stop]:manual/Manual/Sequoiadb_Command/SdbReplicaGroup/stop.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
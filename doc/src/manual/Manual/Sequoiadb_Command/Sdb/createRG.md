##名称##

createRG - 新建复制组

##语法##

**db.createRG(\<name\>)**

##类别##

Sdb

##描述##

该函数用于新建一个复制组，创建后系统自动为复制组分配一个 GroupID。

##参数##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | ------ | ------ | ------ |
| name | string | 复制组名，同一个数据库对象中，复制组名唯一 | 是 |


> **Note:**
>
> 复制组名不能是空串，不能含点（.）或者美元符号（$），并且长度不能超过127B。


##返回值##

函数执行成功时，将返回一个 SdbReplicaGroup 类型的对象。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.0 及以上版本

##示例##

新建名为“group1”的复制组

```lang-javascript
> db.createRG("group1")
```

[^_^]:
     本文使用的所有引用及链接

[list_info]:manual/Manual/List/list.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
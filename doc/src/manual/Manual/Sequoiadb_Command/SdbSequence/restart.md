##名称##

restart - 使序列从指定值开始重新计数

##语法##

**sequence.restart\(\<value\>\)**

##类别##

SdbSequence

##描述##

该函数可以使序列从指定值开始重新计数。指定的起始值可以是序列最小值至最大值范围内的任意值。

##参数##

value（ *number*， *必填* ）

指定的起始值

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.4.2 及以上版本

##示例##

使当前值为 10 的序列从 1 开始重新计数

```lang-javascript
> sequence.getNextValue()
10
> sequence.restart( 1 )
> sequence.getNextValue()
1
```


[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md

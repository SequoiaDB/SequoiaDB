##名称##

killProcess - 杀死指定进程

##语法##

**System.killProcess( \<options\> )**

##类别##

System

##描述##

杀死指定进程

##参数##

| 参数名    | 参数类型 | 默认值 | 描述         | 是否必填 |
| --------- | -------- | ------ | ------------ | -------- |
| options  | JSON   | ---    | 进程信息     | 是       |

options 参数详细说明如下：

| 属性     | 值类型 | 是否<br>必填 | 格式 | 描述 |
| -------- | ------ | -------- | -------------------- | ---------------------------------- |
| pid    | int |     是   | { pid: 31831 }     | 指定 pid                       |

##返回值##

无

##错误##

如果出错则抛异常，并输出错误信息，可以通过 [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) 获取错误信息或通过 [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) 获取错误码。关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v3.2 及以上版本

##示例##

杀死指定进程

```lang-javascript
> System.killProcess( { pid: 31831 } )
```
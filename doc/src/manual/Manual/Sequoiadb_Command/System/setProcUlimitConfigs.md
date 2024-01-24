##名称##

setProcUlimitConfigs - 修改进程资源限制值

##语法##

**System.setProcUlimitConfigs( \<configs\> )**

##类别##

System

##描述##

修改进程资源限制值

##参数##

| 参数名  | 参数类型 | 默认值       | 描述             | 是否必填 |
| ------- | -------- | ------------ | ---------------- | -------- |
| configs  | JSON   | ---    | 新的限制值    | 是       |

configs 参数可修改的字段见 [getProcUlimitConfigs](manual/Manual/Sequoiadb_Command/System/getProcUlimitConfigs.md) 中的示例

##返回值##

无

##错误##

如果出错则抛异常，并输出错误信息，可以通过 [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) 获取错误信息或通过 [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) 获取错误码。关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v3.2 及以上版本

##示例##

修改进程最大内存大小

```lang-javascript
> System.setProcUlimitConfigs( { "max_memory_size": -1 } )
```
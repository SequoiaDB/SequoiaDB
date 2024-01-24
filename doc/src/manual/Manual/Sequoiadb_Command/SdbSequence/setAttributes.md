##名称##

setAttributes - 修改序列的属性

##语法##

**sequence.setAttributes\(\<options\>\)**

##类别##

SdbSequence

##描述##

该函数用于修改序列的当前值、起始值、最小值、最大值等属性。

##参数##

options（ *object*， *必填* ）

通过 options 参数可以修改序列的属性：

- CurrentValue（ *number* ）：序列当前值

  格式：`CurrentValue : <num>`

- StartValue（ *number* ）：序列起始值

  格式：`StartValue : <num>`

- MinValue（ *number* ）：序列最小值

  格式：`MinValue : <num>`

- MaxValue（ *number* ）：序列最大值

  格式：`MaxValue : <num>`

- Increment（ *number* ）：序列每次增加的间隔

  格式：`Increment : <num>`

- CacheSize（ *number* ）：编目节点每次缓存的序列值的数量

  格式：`CacheSize : <num>`

- AcquireSize（ *number* ）：协调节点每次获取的序列值的数量

  格式：`AcquireSize : <num>`

- Cycled（ *boolean* ）：序列值达到最大值或最小值时是否允许循环

  格式：`Cycled : true | false`


##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.4.2 及以上版本

##示例##

修改序列当前值

```lang-javascript
> var sequence = db.createSequence( "IDSequence" )
> sequence.setAttributes( { CurrentValue: 1000 } )
```


[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md

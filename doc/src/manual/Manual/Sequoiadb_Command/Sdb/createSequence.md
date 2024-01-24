##名称##

createSequence - 创建序列对象

##语法##

**db.createSequence\(\<name\>, \[options\]\)**

##类别##

Sdb

##描述##

该函数用于在当前数据库中创建新的序列对象。

##参数##

+ name（ *string*， *必填* ）

	序列名，不能以"SYS"或"$"起始

+ options（ *object*， *选填* ）

	通过 options 参数可以指定序列的属性：

    1. StartValue（ *number* ）：序列的起始值，正序时，默认值为 1；逆序时，默认值为 -1

       格式：`StartValue : <num>`

    2. MinValue（ *number* ）：序列的最小值，正序时，默认值为 1；逆序时，默认值为 -2^63

       格式：`MinValue : <num>`

    3. MaxValue（ *number* ）：序列的最大值，正序时，默认值为 2^63 -1；逆序时，默认值为 -1

        格式：`MaxValue : <num>`

    4. Increment（ *number* ）：序列每次增加的间隔，可以为正整数或负整数；正数表示正序，负数表示逆序；不能为 0，默认值为 1

        格式：`Increment : <num>`

    5. CacheSize（ *number* ）：编目节点每次缓存的序列值的数量，取值须大于 0，默认值为 1000

        格式：`CacheSize : <num>`

    6. AcquireSize（ *number* ）：协调节点每次获取的序列值的数量，取值须大于 0，且小于等于 CacheSize，默认值为 1000

        格式：`AcquireSize : <num>`

    7. Cycled（ *boolean* ）：序列值达到最大值或最小值时是否允许循环，默认值为 false

        格式：`Cycled : true | false`


##返回值##

函数执行成功时，将返回一个 SdbSequence 类型的对象。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`createSequence()` 函数常见异常如下：

| 错误码 | 错误类型                | 可能发生的原因       | 解决办法 |
| ------ | ----------------------- | -------------------- | -------- |
| -323   | SDB_SEQUENCE_EXIST      | 同名序列已存在       | 检查序列是否存在 |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.4.2 及以上版本

##示例##

创建指定选项的序列

```lang-javascript
> var sequence = db.createSequence( "IDSequence", { Cycled: true } )
```


[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md

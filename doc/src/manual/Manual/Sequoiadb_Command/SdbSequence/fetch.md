##名称##

fetch - 获取当前序列的多个连续的序列值

##语法##

**sequence.fetch\(\<num\>\)**

##类别##

SdbSequence

##描述##

该函数用于一次性获取当前序列的多个连续的序列值。需要获取多个值时，调用该函数比多次调用 [getNextValue()][getNextValue] 函数拥有更好的性能。可以指定预期需要取的个数，但返回个数有可能比预期的少。

##参数##

num（ *number*， *必填* ）

预期需要取的序列值个数

##返回值##

函数执行成功时，返回一个包含三个字段的对象。NextValue 表示第一个可用的序列值；ReturnNum 表示返回序列值的个数；Increment 表示序列值每次增加的间隔。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.4.2 及以上版本

##示例##

获取若干连续的序列值

```lang-javascript
> var sequence = db.createSequence( "IDSequence" )
> sequence.fetch( 10 )
{
  "NextValue": 1,
  "ReturnNum": 10,
  "Increment": 1
}
```

打印所获取的序列值

```lang-javascript
> var result = sequence.fetch( 5 ).toObj()
> var nextValue = result.NextValue
> for (var i = 1; i < result.ReturnNum; i++) {
... println( nextValue );
... nextValue += result.Increment;
... }
11
12
13
14
15
```


[^_^]:
     本文使用的所有引用及链接
[getNextValue]:manual/Manual/Sequoiadb_Command/SdbSequence/getNextValue.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md

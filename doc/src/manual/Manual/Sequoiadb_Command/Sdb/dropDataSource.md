##名称##

dropDataSource - 删除数据源

##语法##

**db.dropDataSource( \< Name \> )**

##类别##

Sdb

##描述##

该函数用于删除指定的数据源。用户删除数据源时，需要确保没有数据库对象在使用该数据源，即数据源不关联任何集合空间或集合。

##参数##

Name（ *string，必填* ）

数据源名称

##返回值##

函数执行成功时，无返回值。
 
函数执行失败时，将抛异常并输出错误信息。

##错误##

`dropDataSource()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| ------ | -------- | -------------- | -------- |
| -370   | SDB_CAT_DATASOURCE_NOTEXIST | 数据源不存在 | 检查是否存在指定数据源 |
| -371   | SDB_CAT_DATASOURCE_INUSE   | 数据源已被使用 | 删除数据源时需要删除数据源中关联的集合空间或集合 |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.2.8 及以上版本

##示例##

删除名为“datasource”的数据源

```lang-javascript
> db.dropDataSource("datasource")
```



[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
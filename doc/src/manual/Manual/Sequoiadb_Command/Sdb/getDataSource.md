##名称##

getDataSource - 获取数据源的引用

##语法##

**db.getDataSource( \<name\> )**

##类别##

Sdb

##描述##

该函数用于获取数据源的引用，通过该引用可以对数据源的元数据信息进行修改。

##参数##

name（ *string*， *必填* ）

数据源名称

##返回值##

函数执行成功时，将返回一个 DataSource 对象。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`getDataSource()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| ------ | -------- | -------------- | -------- |
| -370   | SDB_CAT_DATASOURCE_NOTEXIST | 数据源不存在 | 检查是否存在指定数据源 |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.2.8 及以上版本

##示例##

获取数据源 datasource 的引用

```lang-javascript
> var ds = db.getDataSource("datasource")
```

修改该数据源的访问权限为“WRITE”

```lang-javascript
> ds.alter({AccessMode:"WRITE"})
```


[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
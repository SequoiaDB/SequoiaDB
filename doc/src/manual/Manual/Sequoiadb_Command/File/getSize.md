##名称##

getSize - 获取文件的大小

##语法##

**File.getSize(\<filepath\>)**

##类别##

File

##描述##

获取文件的大小。

##参数##

| 参数名   | 参数类型 | 默认值 | 描述     | 是否必填 |
| -------- | -------- | ------ | -------- | -------- |
| filepath | string   | ---    | 文件路径 | 是       |


> Note：

> 无法获取二进制文件的大小。如果想获取二进制文件的大小可以参考[getLenght][Length]。

##返回值##

返回指定文件的大小。

##错误##

如果出错则抛异常，并输出错误信息，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。关于错误处理可以参考[常见错误处理指南][faq]。

常见错误可参考[错误码][error_code]。

##版本##

v3.2 及以上版本

##示例##

获取文件的大小

```lang-javascript
> File.getSize( "/opt/sequoiadb/file.txt" )
13558
```


[^_^]:
    本文使用的所有引用及链接
[Length]:manual/Manual/Sequoiadb_Command/FileContent/getLength.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md


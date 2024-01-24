##名称##

read - 读取文本文件

##语法##

**file.read(\[size\])**

##类别##

File

##描述##

读取文本文件。

##参数##

| 参数名 | 参数类型 | 默认值                             | 描述         | 是否必填 |
| ------ | -------- | ---------------------------------- | ------------ | -------- |
| size   | int      | 默认读取当前文件游标之后的全部内容 | 读取的字节数 | 否       |

##返回值##

返回读取的文件内容。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v3.2 及以上版本

##示例##

* 打开一个文件，获取文件描述符

    ```lang-javascript
    > var file = new File( "/opt/sequoiadb/file" )
    ```

* 读取文本文件内容

    ```lang-javascript
    > file.read()
    SquoiaDB
    ```
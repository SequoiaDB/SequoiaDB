##名称##

readLine - 读取文件的一行数据

##语法##

**file.readLine()**

##类别##

File

##描述##

读取文件的一行数据。

##参数##

无

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
    > var file = new File( "/opt/sequoiadb/file.txt" )
    > file.read()
    0:sequoiadb is wonderful.
    1:wonderful sequoiadb.
    ```

* 读取文件

    ```lang-javascript
    > file.seek(0)
    > file.readLine()
    0:sequoiadb is wonderful. 
    > file.readLine()
    1:wonderful sequoiadb.
    ```
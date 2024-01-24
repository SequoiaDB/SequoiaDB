##名称##

getLength - 获取二进制流的大小

##语法##

**content.getLength()**

##类别##

FileContent

##描述##

获取二进制流的大小

##参数##

无

##返回值##

返回二进制流的大小。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v3.2 及以上版本

##示例##

* 打开一个二进制文件，获取文件描述符

    ```lang-javascript
    > var file = new File( "/opt/trunk/file.dump" )
    ```

* 读取文件内容到 fileContent 对象中（详细可参考命令[File::readContent](manual/Manual/Sequoiadb_Command/File/readContent.md)）

    ```lang-javascript
    > var content = file.readContent( 10000 )
    ```

* 获取二进制流的大小

    ```lang-javascript
    > content.getLength()
    10000
    ```

##名称##

toBase64Code - 将二进制流转化为 base64 编码格式

##语法##

**content.toBase64Code()**

##类别##

FileContent

##描述##

将二进制流转化为 base64 编码格式

##参数##

无

##返回值##

返回二进制流的 base64 编码格式字符串。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v3.2 及以上版本

##示例##

* 打开一个二进制文件，获取文件描述符

    ```lang-javascript
    > var file = new File( "/opt/sequoiadb/file.dump" )
    ```

* 读取文件内容到 fileContent 对象中（详细可参考命令[File::readContent](manual/Manual/Sequoiadb_Command/File/readContent.md)）
  
    ```lang-javascript
    > var content = file.readContent( 10 )
    ```

* 把 fileContent 对象中的内容转化为 base64 编码格式

    ```lang-javascript
    > var base64String = content.toBase64Code()
    ```

* 可以把转换之后的字符串写入一个新建文件中，方便查看该字符串

    ```lang-javascript
    > var base64StringFile = new File( "/opt/sequoiadb/file.dump.base64" ) 
    > base64StringFile.write( base64String )
    ```

* 读取文件 base64StringFile 的内容

    ```lang-javascript
    > base64StringFile.seek(0)
    > base64StringFile.read()
    BQAGAAgA8////w==
    ```

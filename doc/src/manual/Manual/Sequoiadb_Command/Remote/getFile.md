##名称##

getFile - 新建一个远程文件对象

##语法##

***getFile( \<filepath\>, \[permission\], \[mode\] )***

##类别##

Remote

##描述##

新建一个远程文件对象。

##参数##

| 参数名     | 参数类型 | 默认值                                | 描述               | 是否必填 |
| ---------- | -------- | ------------------------------------- | ------------------ | -------- |
| filepath   | string   | ---                                   | 文件路径           | 否       |
| permission | int      | 0700                                  | 设置打开文件的权限 | 否       |
| mode       | int      | SDB_FILE_READWRITE \| SDB_FILE_CREATE | 设置文件打开的方式 | 否       |

> Note：

> 如果想执行远程文件类的全局方法，可以不填任何参数。具体全局方法可在 sdb shell 中执行命令 File.help() 查看。

mode 参数的可选值如下表：

| 可选值                | 描述                         |
| --------------------- | ---------------------------- |
| SDB_FILE_CREATEONLY   | 只创建文件                   |
| SDB_FILE_REPLACE      | 覆盖原文件的内容，写入新内容 |
| SDB_FILE_CREATE       | 创建文件并打开文件           |
| SDB_FILE_READONLY     | 以只读的模式打开文件         |
| SDB_FILE_WRITEONLY    | 以只写的模式打开文件         |
| SDB_FILE_READWRITE    | 以可读可写的模式打开文件     |
| SDB_FILE_SHAREREAD    | 以共享读的模式打开文件       |
| SDB_FILE_SHAREWRITE   | 以共享写的模式打开文件       |

> Note：

> 以上标志位可以使用或运算符 ‘ | ’，按位运算组合使用。

##返回值##

无返回值。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。


常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v3.2 及以上版本

##示例##

* 新建一个远程连接对象

    ```lang-javascript
    > var remoteObj = new Remote( "192.168.20.71", 11790 )
    ```

* 新建一个远程文件对象。

    ```lang-javascript
    > var file = remoteObj.getFile( "/opt/sequoiadb/file", 0777, SDB_FILE_READWRITE | SDB_FILE_CREATE )
    ```

* 读取文本文件内容。（详细可参考[read](manual/Manual/Sequoiadb_Command/File/read.md)）

    ```lang-javascript
    > file.read()
    SquoiaDB
    ```

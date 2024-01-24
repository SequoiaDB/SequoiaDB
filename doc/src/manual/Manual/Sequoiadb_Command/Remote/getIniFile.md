##名称##

getIniFile - 打开一个远程 INI 配置文件

##语法##

**getIniFile( \<filename\>, \[flags\] )**

##类别##

Remote

##描述##

打开一个远程 INI 配置文件。

##参数##

| 参数名     | 参数类型 | 默认值                    | 描述                | 是否必填 |
| ---------- | -------- | ------------------------- | ------------------- | -------- |
| filename   | string   | ---                       | 文件路径            | 是       |
| flags      | int      | SDB_INIFILE_FLAGS_DEFAULT | 解析 ini 配置的选项 | 否       |

flags 参数的可选值如下表：

| 可选值                       | 描述                      |
| ---------------------------- | ------------------------- |
| SDB_INIFILE_NOTCASE          | 不区分大小写              |
| SDB_INIFILE_SEMICOLON        | 支持分号( ; )注释符       |
| SDB_INIFILE_HASHMARK         | 支持井号( # )注释符       |
| SDB_INIFILE_ESCAPE           | 支持转义字符，如：\\n     |
| SDB_INIFILE_DOUBLE_QUOMARK   | 支持带双引号( " )的值     |
| SDB_INIFILE_SINGLE_QUOMARK   | 支持带单引号( ' )的值     |
| SDB_INIFILE_EQUALSIGN        | 支持等号( = )的键值分隔符 |
| SDB_INIFILE_COLON            | 支持冒号( : )的键值分隔符 |
| SDB_INIFILE_UNICODE          | 支持 Unicode 编码         |
| SDB_INIFILE_STRICTMODE       | 开启严格模式，不允许重复的段名和键名 |
| SDB_INIFILE_FLAGS_DEFAULT    | 默认的 flags，等同于 SDB_INIFILE_SEMICOLON \| SDB_INIFILE_EQUALSIGN \| SDB_INIFILE_STRICTMODE |

> Note：  
> 以上标志位可以使用或运算符 "|"，按位运算组合使用。

##返回值##

执行成功，返回一个新的远程 IniFile 对象。

执行失败，抛异常。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v3.2 及以上版本

##示例##

* 创建一个远程连接对象

    ```lang-javascript
    > var remoteObj = new Remote( "sdbserver1", 11790 )
    ```

* 创建一个远程 IniFile 对象。

    ```lang-javascript
    > var ini = remoteObj.getFile( "/opt/sequoiadb/file.ini", SDB_INIFILE_FLAGS_DEFAULT )
    ```

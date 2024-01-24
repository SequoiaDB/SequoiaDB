##名称##

delSectionComment - 删除指定 section 的注释

##语法##

**IniFile.delSectionComment( \<section\> )**

##类别##

IniFile

##描述##

删除指定 section 的注释。

##参数##

| 参数名     | 参数类型 | 默认值  | 描述                            | 是否必填 |
| ---------- | -------- | --------| ------------------------------- | -------- |
| section    | string   | ---     | 段名                            | 是       |

##返回值##

执行成功，无返回值.

执行失败，抛异常。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v3.2 及以上版本

##示例##

* 打开一个 INI 文件。

    ```lang-javascript
    > var ini = new IniFile( "/opt/sequoiadb/file.ini", SDB_INIFILE_FLAGS_DEFAULT )
    ```

* 删除指定 section 的注释。

    ```lang-javascript
    > ini.delSectionComment( "info" )
    ```
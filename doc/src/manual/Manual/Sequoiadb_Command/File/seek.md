##名称##

seek - 移动文件游标

##语法##

**file.seek(\<offset\>,\[where\])**

##类别##

File

##描述##

移动文件游标。

##参数##

| 参数名 | 参数类型 | 默认值 | 描述         | 是否必填 |
| ------ | -------- | ------ | ------------ | -------- |
| offset | int      | ---    | 游标的偏移量 | 是       |
| where  | char     | b      | 移动模式     | 否       |

where 参数可选值如下表：

| 可选值 | 描述                                        |
| ------ | ------------------------------------------- |
|   b    | 文件偏移量为 offset                         |
|   c    | 文件偏移量为当前文件游标的偏移量加上 offset |
|   e    | 文件偏移量为文件的大小加上 offset           |

> Note :

> 当 where 参数为 "e" 时，参数 offset 可以为负数。

##返回值##

无返回值。

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

* 移动文件游标，从文件开头位置执行偏移

    ```lang-javascript
    > file.seek(2)
    > file.read()
    sequoiadb is wonderful.
    1:wonderful sequoiadb.
    > file.seek( 2, "b" )
    > file.read()
    sequoiadb is wonderful.
    1:wonderful sequoiadb.
    ```

* 移动文件游标，从文件当前的游标位置执行偏移

    ```lang-javascript
    > file.seek(2)
    > file.seek( 2, "c" )
    > file.read()
    quoiadb is wonderful.
    1:wonderful sequoiadb.
    ```

* 把游标移至文件末尾

    ```lang-javascript
    > file.seek(0)
    > file.seek( -5, "e" )
    > file.read()
    adb.
    ```

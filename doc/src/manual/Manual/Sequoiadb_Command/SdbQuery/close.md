##名称##

close - 关闭游标

##语法##

**query.close()**

##类别##

SdbQuery

##描述##

关闭游标。

##参数##

无

##返回值##

无返回值。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v3.2 及以上版本。

##示例##

* 关闭游标

    ```lang-javascript
    > var query = db.sample.employee.find()
    > query.close()
    ```

* 获取第一条记录

    ```lang-javascript
    > query[0]
    uncaught exception: -31
    Failed to get next
    ```
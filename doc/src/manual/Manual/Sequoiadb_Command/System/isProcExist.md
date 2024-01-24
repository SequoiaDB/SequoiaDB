##名称##

isProcExist - 判断指定进程是否存在

##语法##

**System.isProcExist( \<options\> )**

##类别##

System

##描述##

判断指定进程是否存在

##参数##

| 参数名    | 参数类型 | 默认值 | 描述         | 是否必填 |
| --------- | -------- | ------ | ------------ | -------- |
| options  | JSON   | ---    | 进程信息     | 是       |

options 参数详细说明如下：

| 属性     | 值类型 | 是否<br>必填 | 格式 | 描述 |
| -------- | ------ | -------- | -------------------- | ----------------- |
| value    | string |     是   | { value: "31831" }     | 指定类型的值     |
| type    | string |     否   | { type: "pid" } 表示根据 pid 判断进程是否存在<br>{ type: "name" } 表示根据服务名判断进程是否存在  | 查询类型      |

##返回值##

存在指定进程返回 true，否则返回 false

##错误##

如果出错则抛异常，并输出错误信息，可以通过 [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) 获取错误信息或通过 [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) 获取错误码。关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v3.2 及以上版本

##示例##

* 通过指定 pid 判断

    ```lang-javascript
    > System.isProcExist( { value: "31831", type: "pid" } )
    true
    ```

* 通过指定服务名判断

    ```lang-javascript
    > System.isProcExist( { value: "sdbcm(11790)", type: "name" } )
    true
    ```
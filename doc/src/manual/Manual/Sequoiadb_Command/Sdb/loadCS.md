[^_^]:
     隐藏接口，不暴露

##名称##

loadCS - 加载集合空间到内存

##语法##

**db.loadCS( \<csName\>, [options] )**

##类别##

Sdb

##描述##

该函数用于加载集合空间到内存。

##参数##

| 参数名  | 参数类型 | 默认值  | 描述               | 是否必填 |
| ------- | -------- | ------- | ------------------ | -------- |
| csName  | string   | ---     | 集合空间名         | 是       |
| options | JSON     | 空      | [命令位置参数](manual/Manual/Sequoiadb_Command/location.md) | 否       |

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v3.2 及以上版本

##示例##

* 查询数据。（假定存在集合空间 “sample”）

    ```lang-javascript
    > db.sample.employee.find()
    {
       "_id": {
         "$oid": "5d36c9d5c6b1cee56abefc7e"
       },
       "name": "fang",
       "age": 18
    }
    ```

* 卸载内存中的集合空间 “sample”。

    ```lang-javascript
    > db.unloadCS( "sample" )
    ```

* 查询数据。

    ```lang-javascript
    > db.sample.employee.find()
    uncaught exception: -34
    Collection space does not exist:
    Collection space[sample] has been unloaded
    ```

* 加载集合空间 “sample” 到内存中。

    ```lang-javascript
    > db.loadCS( "sample" )
    ```

* 再次查询数据。

    ```lang-javascript
    > db.sample.employee.find()
    {
       "_id": {
         "$oid": "5d36c9d5c6b1cee56abefc7e"
       },
       "name": "fang",
       "age": 18
    }
    ```
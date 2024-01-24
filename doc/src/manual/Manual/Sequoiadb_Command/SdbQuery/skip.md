##名称##

skip - 指定结果集从哪条记录开始返回

##语法##

**query.skip( [num] )**

##类别##

SdbQuery

##描述##

指定结果集从哪条记录开始返回。

##参数##

| 参数名 | 参数类型 | 默认值 | 描述                       | 是否必填 |
| ------ | -------- | ------ | -------------------------- | -------- |
| num    | int      | ---    | 自定义结果集从哪条记录返回 | 是       |

>**Note:**

>如果结果集的记录数小于 ( num + 1 )，无记录返回。

##返回值##

返回结果集的游标。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v2.0 及以上版本。

##示例##

* 选择集合 employee 下的记录，从第2条记录开始返回。

    ```lang-javascript
    > db.sample.employee.find().skip(1)
    {
       "_id": {
         "$oid": "5cf8aefe5e72aea111e82b39"
       },
       "name": "ben",
       "age": 21
    }
    {
       "_id": {
         "$oid": "5cf8af065e72aea111e82b3a"
       },
       "name": "alice",
       "age": 19
    }
    ```

* 选择集合 employee 下的记录，从第 4 条记录开始返回。（当前集合只有 3 条记录）

    ```lang-javascript
    > db.sample.employee.find().skip(3)
    Return 0 row(s).
    ```

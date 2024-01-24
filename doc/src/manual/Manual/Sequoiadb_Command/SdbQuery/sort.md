##名称##

sort - 对结果集按指定字段排序

##语法##

**query.sort( \<sort\> )**

##类别##

SdbQuery

##描述##

对结果集按指定字段排序。

##参数##

| 参数名 | 参数类型 | 默认值                 | 描述                       | 是否必填 |
| ------ | -------- | ---------------------- | -------------------------- | -------- |
| sort   | JSON     | 默认不对结果集进行排序 | 对结果集按指定字段排序。字段名的值为1或者-1，1代表升序，-1代表降序 | 是 |

>**Note:**

>当 find() 方法使用 sel 选项，若该选项没有包含 sort() 指定的排序字段，此时 sort() 方法设置的排序无意义，从而被自动忽略。

##返回值##

返回结果集的游标。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v2.0 及以上版本。

##示例##

- 返回集合 employee 中 age 字段值大于20的记录（如使用 [$gt](manual/Manual/Operator/Match_Operator/gt.md) 查询），设置只返回记录的 name 和 age 字段，并按 age 字段值的升序排序。

    ```lang-javascript
      > db.sample.employee.find( { age: { $gt: 20 } }, { age: "", name: "" } ).sort( { age: 1 } )
      {
        "name": "Jack",
        "age": 22
      }
      {
        "name": "Tom",
        "age": 23
      }
      {
        "name": "John",
        "age": 25
      }
    ```

* 指定一个无效的排序字段。

    ```lang-javascript
      > db.sample.employee.find( { age: { $gt: 20 } }, { age: "", name: "" } ).sort( { "sex": 1 } )
      {
        "name": "Jack",
        "age": 22
      }
      {
        "name": "Tom",
        "age": 23
      }
      {
        "name": "John",
        "age": 25
      }
    ```
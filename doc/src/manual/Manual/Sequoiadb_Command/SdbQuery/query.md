##名称##

query - 以下标的方式访问查询结果集

##语法##

**query[ \<index\> ]**

##类别##

SdbQuery

##描述##

以下标的方式访问查询结果集。

##参数##

| 参数名 | 参数类型 | 默认值 | 描述     | 是否必填 |
| ------ | -------- | ------ | -------- | -------- |
| index  | int      | ---    | 数组下标 | 是       |

##返回值##

返回指定下标的记录。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v2.0 及以上版本。

##示例##

```lang-javascript
> var query = db.sample.employee.find()
> println( query[0] )
{
  "_id": {
    "$oid": "5cf8aef75e72aea111e82b38"
  },
  "name": "tom",
  "age": 20
}
```

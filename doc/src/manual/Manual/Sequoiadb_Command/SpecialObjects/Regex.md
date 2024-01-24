
SequoiaDB支持使用正则表达式检索用户数据。

数据类型的介绍可参考 [数据类型](manual/Distributed_Engine/Architecture/Data_Model/data_type.md)。

##Json格式##

* 语法

  *{ "$regex": \<pattern\>, "$options": \<options\> }*

* 参数描述

  | 参数名  | 参数类型  | 描述                 | 是否必填 |
  | ------- | --------- | -------------------- | -------- |
  | pattern | 字符串 | 正则表达式字符串。      | 是       |
  | options | 字符串 | 选项，详细见下表。      | 是       |

  | 选项 | 描述                 |
  | ---- | -------------------- |
  | i    | 匹配时不区分大小写。 |
  | m    | 允许进行多行匹配；当该参数打开时，字符“\^”与“&”匹配换行符的之后与之前的字符。 |
  | x    | 忽略正则表达式匹配中的空白字符；如果需要使用空白字符，在空白字符之前使用反斜线“\”进行转意。 |
  | s    | 允许“.”字符匹配换行符。 |

##函数格式##

* 语法： 

  *Regex( \<pattern\>, \<options\> )*

* 参数描述

  | 参数名  | 参数类型  | 描述                 | 是否必填 |
  | ------- | --------- | -------------------- | -------- |
  | pattern | 字符串 | 正则表达式字符串。      | 是       |
  | options | 字符串 | 选项，详细见下表。      | 是       |

  | 选项 | 描述                 |
  | ---- | -------------------- |
  | i    | 匹配时不区分大小写。 |
  | m    | 允许进行多行匹配；当该参数打开时，字符“\^”与“&”匹配换行符的之后与之前的字符。 |
  | x    | 忽略正则表达式匹配中的空白字符；如果需要使用空白字符，在空白字符之前使用反斜线“\”进行转意。 |
  | s    | 允许“.”字符匹配换行符。 |

##返回值##

无返回值，出错抛异常，并输出错误信息。可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息，通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

##错误##

错误信息记录在节点诊断日志（diaglog）中，可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

| 错误码 | 可能的原因  | 解决方法     |
| ------ | ----------- | ------------ |
| -6     | 参数错误    | 请参考示例。 |

##示例##

使用正则表达式匹配记录

```lang-javascript
> db.sample.employee.insert( { a: "White" } )
> db.sample.employee.insert( { a: "white" } )
> db.sample.employee.insert( { a: "Black" } )
> db.sample.employee.find( { a: { "$regex": "^W", "$options": "i" } } )
{
  "_id": {
    "$oid": "58130a8eeefbe4c22d000006"
  },
  "a": "White"
}
{
  "_id": {
    "$oid": "58130a93eefbe4c22d000007"
  },
  "a": "white"
}
Return 2 row(s).
> db.sample.employee.find( { a: Regex( "^W", "i" ) } )
{
  "_id": {
    "$oid": "58130a8eeefbe4c22d000006"
  },
  "a": "White"
}
{
  "_id": {
    "$oid": "58130a93eefbe4c22d000007"
  },
  "a": "white"
}
Return 2 row(s).
```

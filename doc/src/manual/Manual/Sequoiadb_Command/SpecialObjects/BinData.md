
Base64形式的二进制数据。

在SequoiaDB中的数据使用JSON形式访问，因此对于二进制的数据需要用户使用 Base64方式进行编码，之后以字符串的形式发送至数据库。

数据类型的介绍可参考 [数据类型](manual/Distributed_Engine/Architecture/Data_Model/data_type.md)。

##Json格式##

* 语法

  *{ "$binary": \<data\>, "$type": \<type\> }*

* 参数描述

  | 参数名 | 参数类型 | 描述                 | 是否必填 |
  | ------ | -------- | -------------------- | -------- |
  | data   | 字符串   | base64加密后的内容。 | 是       |
  | type   | 字符串   | 类型。               | 是       |

##函数格式##

* 语法： 

  *BinData( \<data\>, \<type\> )*

* 参数描述

  | 参数名 | 参数类型 | 描述                 | 是否必填 |
  | ------ | -------- | -------------------- | -------- |
  | data   | 字符串   | base64加密后的内容。 | 是       |
  | type   | 字符串   | 类型。               | 是       |

##返回值##

无返回值，出错抛异常，并输出错误信息，可以通过 [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) 获取错误信息 或 通过 [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) 获取错误码。关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md) 。

##错误##

错误信息记录在节点诊断日志（diaglog）中，可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

| 错误码 | 可能的原因  | 解决方法     |
| ------ | ----------- | ------------ |
| -6     | 参数错误    | 请参考示例。 |

##示例##

插入二进制类型的记录

```lang-javascript
> db.sample.employee.insert( { a: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" } } )
> db.sample.employee.insert( { a: BinData( "aGVsbG8gd29ybGQ=", "1" ) } )
```

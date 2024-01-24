
长整型。范围：-9223372036854775808 ~ 9223372036854775807。

数据类型的介绍可参考 [数据类型](manual/Distributed_Engine/Architecture/Data_Model/data_type.md)。

##Json格式##

* 语法

  *{ "$numberLong": \<data\> }*

* 参数描述

  | 参数名 | 参数类型     | 描述                 | 是否必填 |
  | ------ | ------------ | -------------------- | -------- |
  | data   | 字符串       | 指定64位整数。       | 是       |

##函数格式##

* 语法： 

  *NumberLong( \<data\> )*

* 参数描述

  | 参数名 | 参数类型     | 描述                 | 是否必填 |
  | ------ | ------------ | -------------------- | -------- |
  | data   | 字符串       | 指定64位整数。       | 是       |

>**Note:**
>
>数值型参数实际上是double类型，当超过double精度时，可能丢失精度。字符型参数不会丢精度。当操作较大值时，请使用字符串类型的参数。

##返回值##

无返回值，出错抛异常，并输出错误信息。可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息，通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。



##错误##

错误信息记录在节点诊断日志（diaglog）中，可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

| 错误码 | 可能的原因  | 解决方法     |
| ------ | ----------- | ------------ |
| -6     | 参数错误    | 请参考示例。 |

##示例##

插入长整型的记录

```lang-javascript
> db.sample.employee.insert( { a: { $numberLong: "9223372036854775807" } } )
> db.sample.employee.insert( { a: NumberLong( 2147483648 ) } )
> db.sample.employee.insert( { a: NumberLong( "9223372036854775807" ) } )
```



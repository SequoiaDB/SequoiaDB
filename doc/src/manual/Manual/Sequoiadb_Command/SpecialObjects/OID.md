
对象 ID 为一个12字节的BSON 数据类型，包括如下内容：

* 4 字节精确到秒的时间戳
* 3 字节系统（物理机）标示
* 2 字节进程 ID
* 3 字节由随机数起始的序列号

数据类型的介绍可参考 [数据类型](manual/Distributed_Engine/Architecture/Data_Model/data_type.md)。

##Json格式##

* 语法

  *{ "$oid": \<data\> }*

* 参数描述

  | 参数名 | 参数类型 | 描述                 | 是否必填 |
  | ------ | -------- | -------------------- | -------- |
  | data   | 字符串   | 12字节16进制字符串。 | 是       |

##函数格式##

* 语法： 

  *ObjectId( [data] )*

* 参数描述

  | 参数名 | 参数类型 | 描述                 | 是否必填 |
  | ------ | -------- | -------------------- | -------- |
  | data   | 字符串   | 参数为12字节16进制字符串时，生成一个指定值的OID。<br>参数为空时，生成一个随机的OID。 | 否       |

##返回值##

无返回值，出错抛异常，并输出错误信息。可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息，通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

##错误##

错误信息记录在节点诊断日志（diaglog）中，可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

| 错误码 | 可能的原因  | 解决方法     |
| ------ | ----------- | ------------ |
| -6     | 参数错误    | 请参考示例。 |

##示例##

插入OID类型的记录

```lang-javascript
> db.sample.employee.insert( { a: ObjectId() } )
> db.sample.employee.insert( { a: ObjectId( "55713f7953e6769804000001" ) } )
> db.sample.employee.insert( { "_id": { "$oid": "55713f7953e6769804000001" } } )
```

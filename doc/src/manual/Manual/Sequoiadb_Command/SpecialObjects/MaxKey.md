
最大值，所有数据类型中的最大值。

数据类型的介绍可参考 [数据类型](manual/Distributed_Engine/Architecture/Data_Model/data_type.md)。

##Json格式##

* 语法

  *{ "$maxKey": 1 }*

* 参数描述

  无

##函数格式##

* 语法： 

  *MaxKey()*

* 参数描述
  
  无

##返回值##

无返回值，出错抛异常，并输出错误信息，可以通过 [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) 获取错误信息 或 通过 [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) 获取错误码。关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md) 。

##错误##

错误信息记录在节点诊断日志（diaglog）中，可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##示例##

插入最大值类型的记录

```lang-javascript
> db.sample.employee.insert( { a: { "$maxKey": 1 } } )
> db.sample.employee.insert( { a: MaxKey() } )
```

##名称##

dropIndex - 删除集合中指定的索引

##语法##

**db.collectionspace.collection.dropIndex\(\<name\>\)**

##类别##

SdbCollection

##描述##

该函数用于删除集合中指定的[索引][index]。

##参数##

| 参数名 | 类型 | 描述 | 是否必填 |
| ------ | -------- | ---- | -------- |
| name   | string   | 索引名，同一个集合中的索引名必须唯一| 是 |

> **Note:**
>
> - 做删除索引操作时，索引名必须在集合中存在。
> - 索引名不能是空串，含点（.）或者美元符号（$），且长度不超过127B。

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.0 及以上版本

##示例##

删除集合 samle.employee 下名为"ageIndex"的索引，假设该索引已存在

```lang-javascript
> db.sample.employee.dropIndex("ageIndex")
```



[^_^]:
    本文使用的所有引用及链接
[index]:manual/Distributed_Engine/Architecture/Data_Model/index.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md

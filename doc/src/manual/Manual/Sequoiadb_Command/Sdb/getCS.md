##名称##

getCS - 获取指定集合空间

##语法##

**db.getCS(\<name\>)**

##类别##

Sdb

##描述##

该函数用于获取指定集合空间。

##参数##

| 参数名 | 类型   | 描述   | 是否必填 |
| ------ | ------ | ------ | ------ |
| name | string | 集合空间名，同一个数据库对象中集合空间名唯一 | 是 |

> **Note:**
>
> * name 字段的值不能是空串，不能含点（.）或者美元符号（$），且长度不超过 127B。
> * 集合空间在数据库对象中存在。

##返回值##

函数执行成功时，将返回一个 SdbCS 类型的对象。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.0 及以上版本

##示例##

返回集合空间 sample 的引用，假定 sample 已存在

```lang-javascript
> db.getCS("sample")
```

[^_^]:
     本文使用的所有引用及链接

[list_info]:manual/Manual/List/list.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
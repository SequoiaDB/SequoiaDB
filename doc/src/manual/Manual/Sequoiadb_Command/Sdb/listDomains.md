##名称##

listDomains - 列举所有已创建的域

##语法##

**db.listDomains([cond], [sel], [sort])**

##类别##

Sdb

##描述##

该函数用于列举系统中所有由用户创建的域。

##参数##

| 参数名   | 类型    | 描述   													| 是否必填 |
|----------|-------------|----------------------------------------------------------|----------|
| cond     | object   | 匹配条件，只返回符合 cond 的记录，为 null 时，返回所有。 | 否 	   |
| sel      | object   | 选择返回的字段名。为 null 时，返回所有的字段名。         | 否 	   |
| sort     | object   | 对返回的记录按选定的字段排序。1 为升序；-1 为降序。        | 否 	   |


##返回值##

函数执行成功时，将返回一个 SdbCursor 类型的对象。通过该对象获取域的详细信息列表，字段说明可参考 [SYSDOMAINS 集合][SYSDOMAINS]。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][error_guide]。

##版本##

v2.0 及以上版本

##示例##

列举系统中所有由用户创建的域

```lang-javascript
> db.listDomains()
{
  "_id": {
	"$oid": "5811641e3426f0835eef45bf"
  },
  "Name": "mydomain",
  "Groups": [
	{
	  "GroupName": "group1",
	  "GroupID": 1001
	},
	{
	  "GroupName": "group2",
	  "GroupID": 1002
	},
	{
	  "GroupName": "group3",
	  "GroupID": 1000
	}
  ]
}
```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md
[SYSDOMAINS]:manual/Manual/Catalog_Table/SYSDOMAINS.md
##名称##

listGroups -  获取指定域中所有的复制组

##语法##

**domain.listGroups()**

##类别##

SdbDomain

##描述##

该函数用于获取指定域中所有的复制组。

##参数##

无

##返回值##

函数执行成功时，将通过游标（SdbCursor）方式返回指定域所包含的复制组信息。

函数执行失败时，将抛出异常并输出错误信息。


##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.0 及以上版本

##示例##

获取指定域所包含的复制组信息

```lang-javascript
> var domain = db.getDomain("mydomain")
> domain.listGroups()
{
  "_id": {
    "$oid": "5b92291ec5e807b5e32582cc"
  },
  "Name": "mydomain",
  "Groups": [
    {
      "GroupName": "db1",
      "GroupID": 1000
    },
    {
      "GroupName": "db2",
      "GroupID": 1001
    }
  ],
  "AutoSplit": true
}
```


[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[faq]:manual/FAQ/faq_sdb.md
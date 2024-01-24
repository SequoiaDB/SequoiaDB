##名称##

getCataRG - 获取编目复制组的引用

##语法##

**db.getCataRG()**

##类别##

Sdb

##描述##

该函数用于获取编目复制组的引用，用户可以通过该引用对复制组进行相关操作。

##参数##

无

##返回值##

函数执行成功时，返回 SdbReplicaGroup 类型的对象。

函数执行失败时，将抛出异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##示例##

获取编目分区组引用

```lang-javascript
> var rg = db.getCataRG()
```

通过该引用获取复制组的详细信息

```lang-javascript
> rg.getDetail()
{
  "Group": [
    {
      "dbpath": "/opt/sequoiadb/database/catalog/11800",
      "HostName": "u1604-lxy",
      "Service": [
        {
          "Type": 0,
          "Name": "11800"
        },
        {
          "Type": 1,
          "Name": "11801"
        },
        {
          "Type": 2,
          "Name": "11802"
        },
        {
          "Type": 3,
          "Name": "11803"
        }
      ],
      "NodeID": 1,
      "Status": 1
    }
  ],
  "GroupID": 1,
  "GroupName": "SYSCatalogGroup",
  "PrimaryNode": 1,
  "Role": 2,
  "SecretID": 1519705199,
  "Status": 1,
  "Version": 1,
  "_id": {
    "$oid": "5ff52af05c0657cec2f706d5"
  }
}
```


[^_^]:
     本文使用的所有引用和链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[Sequoiadb_error_code]:manual/Manual/Sequoiadb_error_code.md
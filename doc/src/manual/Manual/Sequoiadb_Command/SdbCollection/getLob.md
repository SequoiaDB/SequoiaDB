##名称##

getLob - 读取大对象

##语法##

**db.collectionspace.collection.getLob\(\<oid\>, \<filepath\>, \[forced\]\)**

##类别##

SdbCollection

##描述##

该函数用于读取集合中的大对象。

##参数##

| 参数名 | 类型 | 描述 | 是否必填 |
| ------ | ---- | ---- | -------- |
|  oid   | string | 大对象的唯一标识 | 是 |
| filepath | string | 待写入的本地文件全路径，该文件不需要手动创建 | 是 |
| forced | boolean | 是否强制覆盖已存在的本地文件，默认值为 false，表示不强制覆盖 | 否 |

##返回值##

函数执行成功时，将返回一个 BSONObj 类型的对象。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.4 及以上版本

##示例##

将 oid 为"5435e7b69487faa663000897"的大对象写入本地文件 `/opt/mylob.txt`

```lang-javascript
> db.sample.employee.getLob('5435e7b69487faa663000897', '/opt/mylob.txt')
{
  "LobSize": 0,
  "CreateTime": {
    "$timestamp": "2021-11-10-14.15.46.466000"
  }
}
```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md

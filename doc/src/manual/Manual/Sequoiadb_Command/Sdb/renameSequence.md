##名称##

renameSequence - 修改序列名

##语法##

**db.renameSequence\(\<oldname\>, \<newname\>\)**

##类别##

Sdb

##描述##

该函数用于对当前数据库中已存在的序列进行重命名。

##参数##

+ oldname（ *string*， *必填* ）

    需要修改的序列名

+ newname（ *string*， *必填* ）

    修改后的序列名

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`renameSequence()` 函数常见异常如下：

| 错误码 | 错误类型                | 可能发生的原因       | 解决办法 |
| ------ | ----------------------- | -------------------- | -------- |
| -324   | SDB_SEQUENCE_NOT_EXIST  | 指定序列不存在       | 检查序列是否存在 |
| -323   | SDB_SEQUENCE_EXIST      | 新序列名已被使用     | 修改新序列名 |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.4.2 及以上版本

##示例##

将名为"IDSequence"的序列重命名为"ID_SEQ"

```lang-javascript
> db.renameSequence("IDSequence", "ID_SEQ")
```


[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
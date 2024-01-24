##名称##

removeBackup - 删除数据库备份

##语法##

**db.removeBackup([options])**

##类别##

Sdb

##描述##

该函数用于删除数据库备份。

##参数##

options（ *object，选填* ）

通过 options 参数可以设置备份名、复制组、备份路径等参数：

- GroupID（ *number/array* ）：备份的复制组 ID，缺省为所有复制组

    格式：`GroupID: 1000` 或 `GroupID:[1000, 1001]`

- GroupName（ *string/array* ）：备份的复制组名，缺省为所有复制组

    格式：`GroupName: "data1"` 或 `GroupName: ["data1", "data2"]`

- Name（ *string* ）：备份名称，缺省则删除所有备份

    格式：`Name: "backup-2014-1-1"`

- Path（ *string* ）：备份路径，缺省为配置参数指定的备份路径

    该路径支持通配符（%g/%G：group name、%h/%H:：host name、%s/%S：service name）。

    当在协调节点上执行命令并使用该参数时，需要使用通配符，以避免所有的节点往同一个路径下进行操作而导致未知 IO 错误。

    格式：`Path: "/opt/sequoiadb/backup/%g"`

- IsSubDir（ *boolean* ）：上述 Path 参数所配置的路径是否为配置参数指定的备份路径的子目录，缺省为 false

    如果该参数为 true，则真实的备份目录为：`配置参数中指定的备份目录/Path目录`

    格式：`IsSubDir: false`

- Prefix（ *string* ）：备份前缀名，支持通配符（%g,%G,%h,%H,%s,%S），缺省为空

    格式：`Prefix: "%g_bk_"`

- ID（ *number* ）：备份 ID，缺省为 -1，表示该名字的所有备份

    格式：`ID: -1`

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.8.2 及以上版本增加 ID 参数。

##示例##

删除数据库中备份名为“backup-2014-1-1”的备份信息

```lang-javascript
> db.removeBackup({Name: "backup-2014-1-1"})
```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
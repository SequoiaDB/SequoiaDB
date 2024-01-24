
##名称##

backup - 备份数据库

##语法##

**db.backup([options])**

##类别##

Sdb

##描述##

该函数用于备份整个数据库或指定复制组。

##参数##

options（ *object，选填* ）

通过参数 options 可以设置备份的名称、路径、描述等属性：

- GroupID（ *array* ）：需要备份的复制组 ID，缺省为所有复制组

    格式：`GroupID:1000` 或 `GroupID:[1000, 1001]`

- GroupName（ *string* ）：需要备份的复制组名，缺省为所有复制组

    格式：`GroupName: "data1"` 或 `GroupName: ["data1", "data2"]`

- Name（ *string* ）：备份名称，缺省为“YYYY-MM-DD-HH:mm:SS”时间格式的备份名
 
    格式：`Name: "backup-2014-1-1"`

- Path（ *string* ）：备份路径，缺省为节点配置项参数 [bkuppath][path] 指定的备份路径

    该参数支持通配符 %g/%G（表示 group name）、%h/%H（表示 host name）和 %s/%S（表示 service name）。在协调节点执行备份并指定该参数时，需要使用通配符，避免所有节点都在同一个路径下进行操作，导致未知 IO 错误。

    格式：`Path: "/opt/sequoiadb/backup/%g"`

- IsSubDir（ *boolean*）：参数 Path 所配置的路径，是否为配置参数中所指定备份路径的子目录，缺省为 falsee

    该参数指定为 true 时，备份目录为：“配置参数中所指定的备份目录/参数 Path 所指定的目录”。

    格式：`IsSubDir: false`

- Prefix（ *string* ）：备份前缀名，缺省为空

    该参数支持通配符（%g、%G、%h、%H、%s 和 %S）。

    格式：`Prefix: "%g_bk_"`

- EnableDateDir（ *boolean* ）：是否开启日期子目录功能，缺省为 false

    如果参数取值为 true 则会自动根据当前日期创建名为“YYYY-MM-DD”的子目录。

    格式：`EnableDateDir: false`

- Description（ *string* ）：备份描述

    格式：`Description: "First backup"`

- EnsureInc（ *boolean* ）：是否开启增量备份，缺省为 false

    格式：`EnsureInc: false`

- OverWrite（ *boolean* ）：存在同名备份是否覆盖，缺省为 false

    格式：`OverWrite: false`

- MaxDataFileSize（ *number* ）：指定最大的备份数据文件大小，默认值为 102400，单位为 MB，取值范围为[32, 8388608]，即 32MB~8TB

    超过指定大小的备份数据文件，将被分割成若干符合规格的文件。

    格式：`MaxDataFileSize: 64`

- Compressed（ *boolean* ）：是否开启数据压缩，缺省为 true

    格式：`Compressed: true`

- CompressionType（ *string* ）：压缩格式类型，取值包括"lz4"、"snappy"和"zlib"，缺省为"snappy"

    格式：`CompressionType: "zlib" `

- BackupLog（ *boolean* ）：当全量备份时是否需要备份所有日志，缺省为 false

    格式：`BackupLog: false`

> **Note:**
>
> v2.8.2 及以上版本新增 Compressed、CompressionType 和 BackupLog 参数。

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`backup()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决方法 |
| ------ | ----------------------- | --- | ------ |
| -240   | SDB_BAR_BACKUP_EXIST    | 相同名字的备份已存在 | 先删除该备份或将参数 OverWrite 设置为 true  |
| -241   | SDB_BAR_BACKUP_NOTEXIST | 增量备份对应的全量备份不存在 | 先执行一次全量备份 |
| -70    | SDB_BAR_DAMAGED_BK_FILE | 备份文件已损坏 | - |
| -57    | SDB_DPS_LOG_NOT_IN_BUF  | 增量备份的开始日志已不存在 | 重新执行全量备份后再进行增量备份 |
| -98    | SDB_DPS_CORRUPTED_LOG   | 相同日志 Hash 校验不一致，日志发生变更 | 重新执行全量备份后再进行增量备份 |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v1.2 及以上版本

##示例##

- 对复制组 group1 进行全量备份

    ```lang-javascript
    > db.backup({Name: "backupName", Description: "backup group1", GroupName: "group1"})
    ```

- 指定备节点进行全量备份

    ```lang-javascript
    > db.setSessionAttr({PreferredInstance: "S", PreferredConstraint: "secondaryonly"})
    > db.backup({Name: "backupName", Description: "backup on secondary"})
    ```

[^_^]:
    本文使用的所有引用及连接
[path]:manual/Distributed_Engine/Maintainance/Database_Configuration/configuration_parameters.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md

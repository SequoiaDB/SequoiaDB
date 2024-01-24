##名称##

listBackup - 列举备份信息

##语法##

**db.listBackup([options], [cond], [sel], [sort])**

##类别##

Sdb

##描述##

该函数用于列举当前数据库的备份信息。

##参数##

- options（ *object，选填* ）

    通过参数 options 可以筛选需要列举的备份信息：

    - GroupID（ *number/array* ）：复制组 ID，缺省为所有复制组

        格式：`GroupName: "data1"` 或 `GroupName: ["data1", "data2"]`

    - GroupName（ *string* ）：复制组名，缺省为所有复制组

        格式：`GroupName: "group1"`

    - Name（ *string* ）：备份名，缺省为所有备份

        格式：`Name: "backup-2014-1-1"`

    - Path（ *string* ）：备份路径，缺省为配置参数指定的备份路径

        该参数支持通配符 %g/%G（表示 group name）、%h/%H（表示 host name）和 %s/%S（表示 service name）。

        格式：`Path: "/opt/sequoiadb/backup/%g"`

    - IsSubDir（ *boolean* ）：参数 Path 所配置的路径，是否为配置参数中所指定备份路径的子目录，缺省为 false

        格式：`IsSubDir: false`

    - Prefix（ *string* ）：备份前缀名，缺省为空

        该参数支持通配符（%g、%G、%h、%H、%s 和 %S）。

        格式：`Prefix: "%g_bk_"`

    - Detail（ *boolean* ）：是否显示备份的详细信息，缺省为 false

        格式：`Detail: true`

- cond（ *object，选填* ）

    指定返回记录的过滤条件

- sel（ *object，选填* ）

    指定返回记录的输出字段

- sort（ *object，选填* ）

    对返回记录按选定的字段排序，取值如下：

    - 1：升序
    - -1：降序

##返回值##

函数执行成功时，将返回一个 SdbCursor 对象。通过该对象获取备份详细信息列表，字段说明可查看 [$LIST_BACKUP][LIST_BACKUP]。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.0 及以上版本

##示例##

* 查看数据库配置参数所指定备份路径的备份信息

    ```lang-javascript
    > db.listBackup()
    {
      "Version": 2,
      "Name": "test",
      "ID": 0,
      "NodeName": "vmsvr2-suse-x64-1:20000",
      "GroupName": "db1",
      "EnsureInc": false,
      "BeginLSNOffset": 195652020,
      "EndLSNOffset": 195652068,
      "TransLSNOffset": -1,
      "StartTime": "2017-06-20-13:02:22",
      "LastLSN": 195652020,
      "LastLSNCode": 1845751176,
      "HasError": false
    }
    ```

* 查看其它路径的备份信息

    将数据节点备份至其它路径

    ```lang-javascript
    > var datadb = new Sdb("sdbserver", 20000)
    > datadb.backup({Path: "/tmp/sequoiadb_backup/20000"})
    ```

    连接协调节点查看备份信息

    ```lang-javascript
    > var db = new Sdb("sdbserver", 11810) 
    > db.listBackup({Path: "/tmp/sequoiadb_backup/20000"})
    {
      "Version": 2,
      "Name": "2017-10-26-10:14:11",
      "ID": 0,
      "NodeName": "sdbserver:20000",
      "GroupName": "db1",
      "EnsureInc": false,
      "BeginLSNOffset": -1,
      "EndLSNOffset": 375546828,
      "TransLSNOffset": -1,
      "StartTime": "2017-10-26-10:14:11",
      "LastLSN": -1,
      "LastLSNCode": 0,
      "HasError": false
    }
    ```

    > **Note:**
    >
    > 用户修改备份路径后，执行 listBackup() 需要指定参数 Path，否则在默认路径下查找不到备份信息。


[^_^]:
     本文使用的所有引用及链接
[LIST_BACKUP]:manual/Manual/SQL_Grammar/Monitoring/LIST_BACKUP.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
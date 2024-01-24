辅助类型对象主要用于辅助用户操作 SequoiaDB 巨杉数据库，合理利用辅助类型对象能够极大地简化 SequoiaDB 操作。

辅助类型对象如下：

| 名称 | 描述 |
|------|------|
| [SdbSnapshotOption][SdbSnapshotOption] | 指定快照查询参数 |
| [SdbQueryOption][SdbQueryOption] | 指定记录查询参数 |
| [SdbTraceOption][SdbTraceOption] | 指定 traceOn 监控参数 |
| [CLCount][CLCount] | 统计当前集合符合条件的记录总数 |
| [User][User] | 用于保存用户名和密码 |
| [CipherUser][CipherUser] | 用于保存用户名、密文文件路径、用户加密令牌和集群名 |

[^_^]:
     本文使用的所有引用及链接
[SdbSnapshotOption]:manual/Manual/Sequoiadb_Command/AuxiliaryObjects/SdbSnapshotOption.md
[SdbQueryOption]:manual/Manual/Sequoiadb_Command/AuxiliaryObjects/SdbQueryOption.md
[SdbTraceOption]:manual/Manual/Sequoiadb_Command/AuxiliaryObjects/SdbTraceOption.md
[CLCount]:manual/Manual/Sequoiadb_Command/AuxiliaryObjects/CLCount.md
[User]:manual/Manual/Sequoiadb_Command/AuxiliaryObjects/User.md
[CipherUser]:manual/Manual/Sequoiadb_Command/AuxiliaryObjects/CipherUser.md
[^_^]:
    迁移

为了方便与传统数据库在数据层进行对接，SequoiaDB 巨杉数据库提供多种数据导入及导出的方法，用户可以根据自身需求选择最合适的方案完成数据迁移。

目前，SequoiaDB 支持使用 sdbimprt 工具快速导入 CSV 和 JSON 数据文件目录。同时，SequoiaDB 支持使用 Oracle 官方迁移工具、第三方迁移工具等方式从 DB2、Oracle 中实时同步数据至 SequoiaDB，也支持基于 MySQL 的 binlog Replication 机制实时复制 MySQL 的数据至 SequoiaDB 中。另外，用户还可以使用 sdbexprt 工具将集合中的数据导出到 CSV 或 JSON 数据存储文件中。

通过学习本章，用户可以了解和掌握以下数据迁移方法：

- [CSV 数据文件导入][csv_import]
- [JSON 数据文件导入][json_import]
- [实时第三方数据复制][third_party_realtime]
- [数据导出][export]


[^_^]:
     本文使用的所有引用和链接
[csv_import]:manual/Distributed_Engine/Maintainance/Migration/csv_import.md
[json_import]:manual/Distributed_Engine/Maintainance/Migration/json_import.md
[third_party_realtime]:manual/Distributed_Engine/Maintainance/Migration/third_party_realtime.md
[export]:manual/Distributed_Engine/Maintainance/Migration/export.md
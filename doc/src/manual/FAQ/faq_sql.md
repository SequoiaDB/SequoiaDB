
本文档按照问题分类，对 SQL 实例常见问题的现象、错误码、诊断方案以及修复方法进行描述。

## MySQL/MariaDB实例

在实际操作报错时，服务端返回的错误码包括 MySQL/MariadDB 错误码和 SequoiaDB 错误码，错误码转换方式可参考[错误码][error_code]。

### 网络问题

1. ERROR 1030 (HY000): Got error 40255 from storage engine

    - 实际错误码为 SDB_TOO_MANY_OPEN_FD（-255），文件描述符已达到上限。
    - 问题诊断：
        - 通过对应的协调节点诊断日志，检查 SequoiaDB 的文件句柄数是否达到上限。
        - 先查看配置项 max_connections/open_files_limit 的值，再使用 `lsof` 命令查看 SQL 实例用户已经打开的文件句柄数，检查 SQL 实例的文件句柄数是否达到上限。
   - 问题修复：
        - 如果 SequoiaDB 句柄数达到上限，可参照 [SequoiaDB FAQ][sdb_faq] 中 SDB_TOO_MANY_OPEN_FD 问题的处理方式。
        - 如果 SQL 实例的文件句柄数达到上限，建议将 SQL 实例用户和 SequoiaDB 用户的 open files 设置为"unlimited"，并调整 SQL 实例的 max_connections/open_files_limit 范围。






[^_^]:
     本文使用的所有引用及链接
[error_code]:manual/Database_Instance/Relational_Instance/MySQL_Instance/error_code.md
[sdb_faq]:manual/FAQ/faq_sdb.md
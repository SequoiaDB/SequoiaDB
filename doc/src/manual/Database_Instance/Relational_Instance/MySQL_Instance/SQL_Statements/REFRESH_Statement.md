[^_^]:
    REFRESH 语句

REFRESH 语句用于刷新表的统计信息，用户需确保拥有表的访问权限。目前该语句仅对存储引擎为 SequoiaDB，且统计信息已缓存的表生效。

##语法##

```lang-sql
REFRESH TABLES [tbl_name [, tbl_name] ...] refresh_option

refresh_option: {
    STATS
}
```

>**Note:**
>
> * 如果不指定表名，默认刷新所有表的统计信息。
> * 不支持在存储过程或函数中使用 REFRESH 语句。

##示例##

- 刷新所有表的统计信息

    ```lang-sql
    mysql> REFRESH TABLES STATS;
    ```

- 刷新表 t1 和 t2 的统计信息

    ```lang-sql
    mysql> REFRESH TABLES t1,t2 STATS;
    ```

    查看是否刷新成功，字段 last refresh time 表示最后一次刷新的时间

    ```lang-sql
    mysql> SHOW ENGINE SEQUOIADB STATUS \G
    *************************** 1. row ***************************
    Type: SequoiaDB
    Name: 
    Status: 
    =====================================
    SEQUOIADB MONITOR OUTPUT
    =====================================
    current timestamp: 1677748690(2023-03-02 17:18:10)
    ------------------
    Cached TABLE_SHARE
    ------------------
    Total number of cached table_share: 2
    ./company/t2, last refresh time:1677748681
    ./company/t1, last refresh time:1677748681

    1 row in set (0.00 sec)
    ```
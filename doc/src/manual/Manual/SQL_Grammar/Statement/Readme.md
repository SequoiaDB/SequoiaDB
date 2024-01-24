SequoiaDB 巨杉数据库可以通过 SQL 语句进行数据操作，支持的语句如下：

| 语句 | 描述 | 示例 |
|------|------|------|
|[create collectionspace][create_collectionspace]| 创建数据库中的集合空间 | db.execUpdate("create collectionspace sample") |
|[drop collectionspace][drop_collectionspace]| 删除数据库中指定的集合空间 | db.execUpdate("drop collectionspace sample") | 
|[create collection][create_collection]| 创建集合 | db.execUpdate("create collection sample.employee")|
|[drop collection][drop_collection]| 删除集合 | db.execUpdate("drop collection sample.employee") |
|[create index][create_index]| 创建索引 | db.execUpdate("create index test_index1 on sample.employee (age)") |
|[drop index][drop_index]| 删除索引 | db.execUpdate("drop index test_index on sample.employee")|
|[list collectionspaces][list_collectionspaces]| 枚举数据库中所有的集合空间 | db.exec("list collectionspaces") |
|[list collections][list_collections]| 枚举数据库中所有的集合 | db.exec("list collections") |
|[insert into][insert_into]| 向集合中插入数据 | db.execUpdate("insert into sample.employee(age,name) values(25,"Tom")") |
|[select][select]| 查询数据 | db.exec( "select age,name from sample.employee" ) |
|[update][update]| 更新数据 | db.execUpdate( "update sample.employee set age=20" ) |
|[delete][delete]| 删除数据 | db.execUpdate("delete from sample.employee") |
|[transaction][transaction]| 事务操作 | db.execUpdate( "begin transaction" ) |

[^_^]:
    本文使用到的所有链接及引用
[create_collectionspace]:manual/Manual/SQL_Grammar/Statement/create_collectionspace.md
[drop_collectionspace]:manual/Manual/SQL_Grammar/Statement/drop_collectionspace.md
[create_collection]:manual/Manual/SQL_Grammar/Statement/create_collection.md
[drop_collection]:manual/Manual/SQL_Grammar/Statement/drop_collection.md
[create_index]:manual/Manual/SQL_Grammar/Statement/create_index.md
[drop_index]:manual/Manual/SQL_Grammar/Statement/drop_index.md
[list_collectionspaces]:manual/Manual/SQL_Grammar/Statement/list_collectionspaces.md
[list_collections]:manual/Manual/SQL_Grammar/Statement/list_collections.md
[insert_into]:manual/Manual/SQL_Grammar/Statement/insert_into.md
[select]:manual/Manual/SQL_Grammar/Statement/select.md
[update]:manual/Manual/SQL_Grammar/Statement/update.md
[delete]:manual/Manual/SQL_Grammar/Statement/delete.md
[transaction]:manual/Manual/SQL_Grammar/Statement/transaction.md
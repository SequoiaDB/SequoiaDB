序列是生成唯一序列值的对象。与自增字段相比，序列没有指定表的限制，可以同时绑定多个表，也可以单独使用。在高可用下，序列的值默认只保证趋势递增（或递减），不保证连续分配。如果多个实例同时插入数据，在小的区间内，可能会出现后插入记录的字段值比先插入的小，但在大的区间内，数值是递增的。

![sequence][diagram]

##基本操作##

以下列举一些简单的操作示例，具体可参考 [MariaDB 官网][mariadb]。

- **创建序列**

   ```lang-sql
   CREATE SEQUENCE s INCREMENT 1 MINVALUE 1 NOMAXVALUE START 1 CACHE 1000 CYCLE;
   ```

- **绑定序列**

   ```lang-sql
   CREATE TABLE t1 (a INT NOT NULL DEFAULT (NEXTVAL(s)), b INT, key(a));
   ```

- **获取序列值**

   ```lang-sql
   SELECT NEXTVAL(s);     /* 获取下一个序列值 */
   SELECT LASTVAL(s);    /* 获取当前序列值 */
   ```

- **设置序列值**

   ```lang-sql
   SELECT SETVAL(s, 100);    /* 设置当前序列值为 100 */
   ALTER SEQUENCE s RESTART 10;    /* 重置序列值为 10 */
   ```

   > **Note:**
   >
   > 在高可用下，SETVAL 操作仅对当前实例有效，不影响其他实例缓存的序列值，而 ALTER SEQUENCE ... RESTART 操作，会清空其他实例已缓存的序列值。

- **修改序列属性**

   ```lang-sql
   ALTER SEQUENCE s CACHE 2000;    /* 修改序列的缓存值数为 2000 */
   ```

- **其他操作**

   ```lang-sql
   ALTER TABLE s RENAME TO s1;    /* 修改序列名为 s1 */
   SHOW CREATE SEQUENCE s1;    /* 查看序列属性 */
   DROP SEQUENCE s1;    /* 删除序列 */
   ```


[^_^]:
     本文使用的所有链接和引用
[diagram]:images/Database_Instance/Relational_Instance/MariaDB_Instance/Operation/sequence_diagram.png
[mariadb]:https://mariadb.com/kb/en/sequence-overview/
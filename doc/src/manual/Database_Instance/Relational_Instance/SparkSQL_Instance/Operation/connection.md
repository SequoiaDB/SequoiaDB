[^_^]:
    SparkSQL 实例-连接


用户可通过 Spark 自带的命令行工具 spark-shell、spark-sql 或 beeline 实现 SparkSQL 实例访问 SequoiaDB 巨杉数据库。

##spark-shell##

1. 进入 spark 部署目录启动 spark-shell

    ```lang-bash
    $ cd spark
    $ bin/spark-shell
    ```
2. 关联 SequoiaDB 的集合空间和集合

    假设 SequoiaDB 中集合名为 test.test，协调节点在 sdbserver1 和 sdbserver2 上，通过 spark-shell 创建一个表 test 来对应 SequoiaDB 的集合

    ```lang-sql
    scala> sqlContext.sql("CREATE TABLE test ( c1 int, c2 string, c3 int ) USING com.sequoiadb.spark OPTIONS ( host 'sdbserver1:11810,sdbserver2:11810', collectionspace 'test', collection 'test')")
    ```

    > **Note:**
    >
    > 创建表所使用的 OPTIONS 参数说明可参考[使用][usage]章节。 

##spark-sql##

1. 进入 spark 部署目录启动 spark-sql

    假设 spark master 所在主机名为 sparkServer

    ```lang-bash
    $ cd spark
    $ bin/spark-sql --master spark://sparkServer:7077
    ```

    >**Note:**
    >
    > 如果不设置--master，将在启动 spark-sql 的机器以 local 方式运行。

2. 关联 SequoiaDB 的集合空间和集合
 
    假设 SequoiaDB 中集合名为 test.test，协调节点在 sdbserver1 和 sdbserver2 上，通过 spark-sql 创建一个表 test 来对应 SequoiaDB 的集合

    ```lang-sql
    spark-sql> CREATE TABLE test( c1 int, c2 string, c3 int ) USING com.sequoiadb.spark OPTIONS ( host 'sdbserver1:11810,sdbserver2:11810', collectionspace 'test', collection 'test')；
    ```

3. 创建表或视图之后就可以在表上执行 SQL 语句，以查询操作为例：

    ```lang-sql
    spark-sql> select * from test;
    ```

    显示 SequoiaDB 集合中的数据信息
   
    ```lang-text  
    0	 mary	 15
    1	 lili	 25
    ```

##beeline##

1. 启动 thriftserver，使其运行在 spark 集群中

    假设启动 thrift 服务的主机为 sparkServer 

    ```lang-bash
    $ ./sbin/start-thriftserver.sh --master spark://sparkServer:7077 
    ```

    >**Note:**
    >
    > thriftserver 是 JDBC 的接口，用户可以通过 JDBC 连接 thriftserver 来访问数据。

2. 启动 beeline 并连接 thrift 服务，thrift 服务默认服务端口为 10000

    ```lang-bash
    $ ./bin/beeline -u jdbc:hive2://sparkServer:10000 
    ```

3. 使用直线脚本测试 thrift JDBC/ODBC server

    ```lang-bash
    $ ./bin/beeline
    ```

4. 在直线脚本连接 JDBC/ODBC server in beeline

    ```lang-bash
    beeline> !connect jdbc:hive2://localhost:10000
    ```

    显示成功连接

    ```lang-text
    Connecting to jdbc:hive2://localhost:10000
    ```

    >**Note:** 
    >
    > beeline 直线脚本会询问用户名和密码。在非安全模式下，简单输入 username 和空白密码即可。在安全模式下，应按照 beeline documentation 下的说明来执行。

5. 在 beeline 命令行窗口执行创表语句

    ```lang-sql
    0: jdbc:hive2://localhost:10000> create table test ( c1 int, c2 string, c3 int) using com.sequoiadb.spark options(host 'sdbserver1:11810,sdbserver2:11810', collectionspace 'test', collection 'test', username 'sdbadmin', password '/opt/spark/conf/sdb.passwd', passwordtype='file'); 
    ```

6. 连接 SequoiaDB 数据库访问映射表数据

    ```lang-sql
    0: jdbc:hive2://localhost:10000> select * from test;
    ```

    显示 SequoiaDB 集合中的数据信息

    ```lang-text
    +-----+-------+-----+--+
    | c1  |  c2   | c3  |
    +-----+-------+-----+--+
    | 0   | mary  | 15  |
    | 1   | lili  | 25  |
    +-----+-------+-----+--+
    ```


[^_^]:
    本文使用的所有引用及链接
[usage]:manual/Database_Instance/Relational_Instance/SparkSQL_Instance/Operation/usage.md
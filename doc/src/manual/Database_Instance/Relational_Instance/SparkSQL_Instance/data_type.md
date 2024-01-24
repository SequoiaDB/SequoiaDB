[^_^]:
    SparkSQL 实例-数据类型映射表

本文档主要介绍存储类型与 SparkSQL 实例类型映射，以及 SequoiaDB 存储引擎向 SparkSQL 实例类型转换的兼容性。

##存储类型与 SparkSQL 实例类型映射##

| 存储引擎类型 | SparkSQL 实例类型   | SQL 实例类型  |
| ------------| ------------------ | ------------ |
|int32|IntegerType|int|
|int64|LongType|bigint|
|double|DoubleType|double|
|decimal|DecimalType|decimal|
|string|StringType|string|
|ObjectId|StringType|string|
|boolean|BooleanType|boolean|
|date|DateType|date|
|timestamp|TimestampType|timestamp|
|binary|BinaryType|binary|
|null|NullType|null|
|BSON(嵌套对象)|StructType|struct\<field:type,…\>|
|array|ArrayType|array\<type\>|

>**Note:**
> 
> 关于 SparkSQL 日期、Timestamp 类型取值范围：
>
> - DATE 类型取值范围为：0000-01-01 至 9999-12-31
> - TIMESTAMP 类型取值范围为：0000-01-01 00:00:00.000000 至 9999-12-31 12:59:59.999999
> 
> 注意：SparkSQL 中 TIMESTAMP 类型取值范围大于 SequoiaDB 中 timestamp 类型取值范围（1902-01-01 00:00:00.000000 至 2037-12-31 23:59:59.999999），如果写入数据超出 SequoiaDB timestamp 类型取值范围则会发生溢出导致数据不准确。

##SequoiaDB 存储引擎向 SparkSQL 实例类型转换的兼容性##

Y 表示兼容，N 表示不兼容

|ByteType | ShortType | IntegerType | LongType | FloatType | DoubleType | DecimalType | BooleanType|
| -----| ---- | ----- | ----- | ----- | ----- | ----- | ----- |
|int32|Y|Y|Y|Y|N|N|Y|N|
|int64|Y|Y|Y|Y|N|N|Y|N|
|double|Y|N|N|Y|N|N|Y|N|
|decimal|Y|Y|Y|Y|N|N|Y|N|
|string|Y|Y|Y|Y|N|N|Y|N|
|ObjectId|Y|N|N|Y|N|N|Y|N|
|boolean|Y|N|N|Y|N|N|Y|N|
|date|Y|Y|Y|Y|N|N|Y|N|
|timestamp|Y|Y|Y|Y|N|N|Y|N|
|binary|Y|N|N|Y|N|N|Y|N|
|null|Y|Y|Y|Y|Y|Y|Y|Y|
|BSON|Y|N|N|N|N|Y|Y|Y|
|array|Y|N|N|N|Y|N|Y|N|

>**Note:**
>
>- 不支持 SparkSQL 的 CalendarIntervalType 类型；
>- null 转换为任意类型仍为 null；
>- 不兼容类型转换时变为目标类型的零值；
>- date 和 timestamp 与数值类型转换时取其毫秒值；
>- string 如果是数值的字符串类型，则可转为对应的数值时，否则转换为 null；
>- boolean 值转为数值类型时，true 为 1，false 为 0；
>- 数值类型之间转换可能会溢出或损失精度。


##时间类型的准确性问题##

###Spark 时间类型不准确###

Spark 2.x 使用 java.sql.Date/java.sql.Timestamp 类型处理时间数据，但不幸的是这两种类型与其父类 java.util.Date 一样，在国际化的处理上不够完善。这会导致部分时间数据在 Spark 与 SequoiaDB 中不一致，如上海时区（东八区） 1900 年之前的时间数据。

###Spark 的处理###

Spark 3.x 引入了 Java8 的新日期类型 java.time.LocalDate/java.time.Instant 处理时间数据，以解决上述时间类型不准确问题。

> **Note**
>
> Spark 3.x 新增参数 spark.sql.datetime.java8API.enabled 用于控制处理时间数据的日期类型，开启该参数时，Spark 才会使用 java.time.LocalDate/java.time.Instant 类型处理时间数据。

###使用建议###

为了保证时间数据在 Spark 与 SequoiaDB 中的一致性，建议：

1. 升级 Spark 框架至 3.x 版本，并搭配使用 Spark 3.0 的连接器

2. 开启参数 spark.sql.datetime.java8API.enabled，以 spark-sql 为例：

    ```sql-lang
    spark-sql> set spark.sql.datetime.java8API.enabled=true;
    ```

###关于时区###

当开启参数 spark.sql.datetime.java8API.enabled 后，Timestamp 类型对应的 java 类型为 java.time.Instant，该类型的处理不再依赖于 JVM 的默认时区，而是依赖于 Spark 参数 spark.sql.session.timeZone 设置的 session 时区。

以 SequoiaDB 中存储的 Timestamp 数据为例：

```shell
> db.sample.employee.find()
{
    "_id": {
        "$oid": "6278924d9da46d7dba91a618"
    },
    "ts": {
        "$timestamp": "1970-01-01-00.00.00.123456"
    }
}
```

1. 创建 spark-sql 映射表映射到 SequoiaDB 集合 sample.employee

    ```sql-lang
    spark-sql> create table test(ts timestamp) using com.sequoiadb.spark options (host 'sdbserver:11810', collectionspace 'sample', collection 'employee', username 'sdbadmin', password 'sdbadmin');
    ```

2. 数据查询会根据具体的 session 时区将 SequoiaDB 中存储的 UTC 时间转换为对应时区的时间

    ```sql-lang
    # 上海时区
    spark-sql> set spark.sql.session.timeZone=Asia/Shanghai;
    spark-sql> select * from test;
    1970-01-01 00:00:00.123456

    # UTC
    spark-sql> set spark.sql.session.timeZone=UTC;
    spark-sql> select * from test;
    1969-12-31 16:00:00.123456

    # 洛杉矶时区
    spark-sql> set spark.sql.session.timeZone=America/Los_Angeles;
    spark-sql> select * from test;
    1969-12-31 08:00:00.123456
    ```

3. 数据写入会根据具体的 session 时区将时间转换为不带时区的 UTC 时间

    ```sql-lang
    # 上海市区
    spark-sql> set spark.sql.session.timeZone=Asia/Shanghai;
    spark-sql> insert into test values (TIMESTAMP '1970-01-01 00:00:00.123456');

    # 洛杉矶时区
    spark-sql> set spark.sql.session.timeZone=America/Los_Angeles;
    spark-sql> insert into test values (TIMESTAMP '1970-01-01 00:00:00.123456');
    ```

    写入结果分别为：

    ```shell
    # 上海时区
    > db.sample.employee.find()
    {
        "_id": {
            "$oid": "627899b0c2dc5f404a5faac4"
        },
        "ts": {
            "$timestamp": "1970-01-01-00.00.00.123456"
        }
    }

    # 洛杉矶时区
    > db.sample.employee.find()
    {
        "_id": {
            "$oid": "62789a55c2dc5f404a5faac5"
        },
        "ts": {
            "$timestamp": "1970-01-01-16.00.00.123456"
        }
    }
    ```

[^_^]:
    FlinkSQL 连接器-数据类型映射

本文档主要介绍 SequoiaDB 巨杉数据库与 FlinkSQL 的数据类型映射，以及两者间数据类型转换的兼容性。
 
##数据类型映射表##

| SequoiaDB 数据类型   |     FlinkSQL 数据类型    |
| ---------------     | ---------------------   |
|   int32             |           INT           |
|   int64             |          BIGINT         |
|   double            |          DOUBLE         |
|   decimal           |         DECIMAL         |
|   string            |   CHAR/VARCHAR/STRING   |
|   OID               |          STRING         |
|   boolean           |         BOOLEAN         |
|   date              |           DATE          |
|   timestamp         | TIMESTAMP_LTZ/TIMESTAMP |
|   binary            | BINARY/VARBINARY/BYTES  |
|   null              |         不支持           |

>**Note:**
> 
> 关于 FlinkSQL 日期、Timestamp 类型取值范围：
>
> - DATE 类型取值范围为：0000-01-01 至 9999-12-31
> - TIMESTAMP_LTZ 类型取值范围为：0000-01-01 00:00:00.000000000 +14:59 至 9999-12-31 23:59:59.999999999 -14:59
> - TIMESTAMP 类型取值范围为：0000-01-01 00:00:00.000000000 至 9999-12-31 12:59:59.999999999
> 
> 注意：Flink SQL 中 TIMESTAMP_LTZ/TIMESTAMP 类型取值范围大于 SequoiaDB 中 timestamp 类型取值范围（1902-01-01 00:00:00.000000 至 2037-12-31 23:59:59.999999），如果写入数据超出 SequoiaDB timestamp 类型取值范围则会发生溢出导致数据不准确。


##数据类型兼容表##

|SequoiaDB\FlinkSQL|TINYINT|SMALLINT|INT|BIGINT|FLOAT|DOUBLE|DATE|TIMESTAMP_LTZ/TIMESTAMP|BOOLEAN|DECIMAL|CHAR/VARCHAR/STRING|BINARY/VARBINARY/BYTES|
|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
|**int32**|可能溢出|可能溢出|Y|Y|Y|Y|可能超出DATE范围|可能超出TIMESTAMP范围|Y|Y|Y|Y|
|**int64**|可能溢出|可能溢出|可能溢出|Y|Y|Y|可能超出DATE范围|可能超出TIMESTAMP范围|Y|Y|Y|Y|
|**double**|可能精度丢失|可能精度丢失|可能精度丢失|可能精度丢失|可能精度丢失|Y|N|N|Y|Y|Y|Y|
|**decimal**|可能精度丢失|可能精度丢失|可能精度丢失|可能精度丢失|可能精度丢失|可能精度丢失|N|N|Y|Y|Y|Y|
|**string**|支持数值字符串|支持数值字符串|支持数值字符串|支持数值字符串|支持数值字符串|支持数值字符串|支持Date字符串|支持Timestamp字符串|支持Bool字符串|支持数值字符串|Y|Y|
|**OID**|N|N|N|N|N|N|N|N|N|N|Y|Y|
|**boolean**|Y|Y|Y|Y|Y|Y|N|N|Y|Y|Y|Y|
|**date**|可能溢出|可能溢出|可能溢出|Y|可能精度丢失|可能精度丢失|Y|Y|N|Y|Y|Y|
|**timestamp**|可能溢出|可能溢出|可能溢出|Y|可能精度丢失|可能精度丢失|Y|Y|N|Y|Y|Y|
|**binary**|N|N|N|N|N|N|N|N|N|N|Y|Y|

>**Note:**
>
> - 不兼容的数据类型发生转换时，原数据将转换为目标类型的零值。
> - string 类型支持将数值字符串转换为 TINYINT、SMALLINT、INT、BIGINT、FLOAT、DOUBLE 等数值类型。如果转换发生溢出，则转换为 null。
> - string 类型支持将"yyyy-MM-dd.HH:mm:ss"格式的日期字符串，转换为 DATE 或 TIMESTAMP 类型，格式不匹配时将转换为零值。
> - boolean 类型转换为数值类型时，true 对应 1，false 对应 0。
> - Flink SQL 中 DECIMAL(p,s) 支持最大精度为 38，数值超出精度表示范围时，视为数据类型不兼容。仅小数部分超出精度时，将舍弃小数部分。

##关于时区##

Flink SQL 为时间戳数据提供了两种数据类型：TIMESTAMP_LTZ、TIMESTAMP。

- TIMESTAMP_LTZ：带时区的时间戳，该类型的数据在进行计算和显示时，会根据当前会话配置的时区转换为对应时区的时间。
- TIMESTAMP：不带时区的时间戳，即 SequoiaDB 中的数据最终会以不带时区的 UTC 时间进行计算和显示。

会话时区配置方式如下：

```sql-lang
-- 设置为 UTC 时区
Flink SQL> SET 'table.local-time-zone' = 'UTC';

-- 设置为上海时区
Flink SQL> SET 'table.local-time-zone' = 'Asia/Shanghai';

-- 设置为洛杉矶时区
Flink SQL> SET 'table.local-time-zone' = 'America/Los_Angeles';
```

>**使用建议:** 
>
>从使用角度来看，TIMESTAMP_LTZ 类型更加符合用户在不同时区下的使用习惯。
> 
>另外，SequoiaDB 在存储 timestamp 类型数据的过程，会把时间数据由本地时间转为不带时区的 UTC 时间，然后再存储。如果用户需要将数据转换为对应时区的时间数据，建议使用 TIMESTAMP_LTZ 类型并将会话时区修改为对应时区来实现。

下面以从 SequoiaDB 读取 timestamp 数据并映射为 TIMESTAMP_LTZ 为例：

1. SequoiaDB 中存储的 timestamp 数据（机器时区为 Asia/Shanghai）

    ```shell
    > db.sample.employee.find()
    {
      "_id": {
        "$oid": "629d79fadb363e6e130723d2"
      },
      "ts": {
        "$timestamp": "1970-01-01-00.00.00.123456"
      }
    }
    ```

2. 创建 Flink SQL 映射表映射到 SequoiaDB 集合 sample.employee，使用 TIMESTAMP_LTZ 进行类型映射。

    ```sql-lang
    -- 使用 TIMESTAMP_LTZ 进行类型映射
    Flink SQL> CREATE TABLE test_ltz (
            >     ts TIMESTAMP_LTZ
            > )
            > WITH (
            >     'connector'='sequoiadb',
            >     'hosts'='sdbserver:11810',
            >     'collectionspace'='sample',
            >     'collection'='employee'   
            > );
    ```

3. 执行查询。Flink 会根据会话配置的时区将 SequoiaDB 中存储的 UTC 时间转换为对应时区的时间。

    ```shell
    # ======= 带时区时间戳类型数据显示效果 ======
    Flink SQL> SET 'table.local-time-zone' = 'UTC';
    Flink SQL> SELECT * FROM test_ltz;
    +----------------------------+
    |             ts             |
    +----------------------------+
    | 1969-12-31 16:00:00.123456 |
    +----------------------------+

    Flink SQL> SET 'table.local-time-zone' = 'Asia/Shanghai';
    Flink SQL> SELECT * FROM test_ltz;
    +----------------------------+
    |             ts             |
    +----------------------------+
    | 1970-01-01 00:00:00.123456 |
    +----------------------------+

    Flink SQL> SET 'table.local-time-zone' = 'America/Los_Angeles';
    Flink SQL> SELECT * FROM test_ltz;
    +----------------------------+
    |             ts             |
    +----------------------------+
    | 1969-12-31 08:00:00.123456 |
    +----------------------------+
    ```
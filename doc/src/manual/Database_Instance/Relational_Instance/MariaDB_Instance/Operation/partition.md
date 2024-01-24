分区功能用于将一张表的存储分散到多个物理位置，达到更好的并发读写效果。在数据量大时，速度提升更为明显。MariaDB 提供了四种分区的方式：RANGE 分区、LIST 分区、HASH 分区和 KEY 分区，同时支持复合分区。

##RANGE 分区##

- **RANGE(\<expression\>)**

    该分区方式根据表达式进行范围分区，记录将根据表达式中值的范围决定坐落的分区，表达式的值必须是整数。以下示例是将 goods 表根据生产年份划分：

    ```lang-sql
    MariaDB [company]> CREATE TABLE goods (
        id INT NOT NULL,
        produced_date DATE,
        name VARCHAR(100),
        company VARCHAR(100)
    )
    PARTITION BY RANGE (YEAR(produced_date)) (
        PARTITION p0 VALUES LESS THAN (1990),
        PARTITION p1 VALUES LESS THAN (2000),
        PARTITION p2 VALUES LESS THAN (2010),
        PARTITION p3 VALUES LESS THAN (2020)
    );
    ```

- **RANGE COLUMNS(\<column_list\>)**

    该分区方式根据列进行范围分区，记录将根据列的值计算分区，可以指定一个列或多个列，不限制列的类型。一般推荐使用 `RANGE COLUMS(<column_list>)` 代替 `RANGE(<expression>)` 分区，因为前者可以达到更好的性能。以下示例是将 goods 表根据生产日期划分：

    ```lang-sql
    MariaDB [company]> CREATE TABLE goods (
        id INT NOT NULL,
        produced_date DATE,
        name VARCHAR(100),
        company VARCHAR(100)
    )
    PARTITION BY RANGE COLUMNS (produced_date) (
        PARTITION p0 VALUES LESS THAN ('1990-01-01'),
        PARTITION p1 VALUES LESS THAN ('2000-01-01'),
        PARTITION p2 VALUES LESS THAN ('2010-01-01'),
        PARTITION p3 VALUES LESS THAN ('2020-01-01')
    );
    ```

    指定多个列时可以使用如下方式：

    ```lang-sql
    MariaDB [company]> CREATE TABLE simple (
        a INT,
        b INT,
        c CHAR(3),
        d CHAR(10)
    )
    PARTITION BY RANGE COLUMNS (a, b, c) (
        PARTITION p0 VALUES LESS THAN (10, 100, 'aaa'),
        PARTITION p1 VALUES LESS THAN (20, 200, 'fff'),
        PARTITION p2 VALUES LESS THAN (30, 300, 'lll'),
        PARTITION p3 VALUES LESS THAN (MAXVALUE, MAXVALUE, MAXVALUE)
    );
    ```

##LIST 分区##

- **LIST(\<expression\>)**
    
    该分区方式根据表达式进行枚举值分区，表达式返回值必须是整数。以下示例是根据业务标签进行分区：

    ```lang-sql
    MariaDB [company]> CREATE TABLE business (
        id INT NOT NULL,
        start DATE NOT NULL DEFAULT '1970-01-01',
        end DATE NOT NULL DEFAULT '9999-12-31',
        COMMENT VARCHAR(255),
        flag INT
    )
    PARTITION BY LIST (flag) (
        PARTITION p0 VALUES IN (1, 3),
        PARTITION p1 VALUES IN (2, 5, 7),
        PARTITION p2 VALUES IN (4, 6),
        PARTITION p3 default
    );
    ```

- **LIST COLUMNS(\<column_list\>)**

    该分区方式根据列进行枚举值分区，可以指定一个列或多个列，不限定列的类型。以下示例是根据业务得分进行分区：

    ```lang-sql
    MariaDB [company]> CREATE TABLE business (
        id INT NOT NULL,
        start DATE NOT NULL DEFAULT '1970-01-01',
        end DATE NOT NULL DEFAULT '9999-12-31',
        COMMENT VARCHAR(255),
        score CHAR(1)
    )
    PARTITION BY LIST COLUMNS (score) (
        PARTITION good VALUES IN ('S', 'A'),
        PARTITION normal VALUES IN ('B', 'C'),
        PARTITION fail VALUES IN ('D')
    );
    ```

##HASH 分区##

- **HASH(\<expression\>)**

    该分区方式根据表达式中的字段计算 hash 值，利用 hash 值进行分区从而均匀打散记录，表达式的返回值必须为整数。由于 SequoiaDB 有独立的 hash 算法，所以这种分区下 SequoiaDB 只取表达式中的列进行计算。一般推荐使用 `KEY(<column_list>)` 代替 `HASH(<expression>)` 语法，因为前者更易于使用。

    ```lang-sql
    MariaDB [company]> CREATE TABLE goods (
        id INT NOT NULL,
        produced_date DATE,
        name VARCHAR(100),
        company VARCHAR(100)
    )
    PARTITION BY HASH (YEAR(produced_date))
    PARTITIONS 4;
    ```

    上述语句对应的 KEY 分区语句如下：

    ```lang-sql
    MariaDB [company]> CREATE TABLE goods (
        id INT NOT NULL,
        produced_date DATE,
        name VARCHAR(100),
        company VARCHAR(100)
    )
    PARTITION BY KEY (produced_date)
    PARTITIONS 4;
    ```

    `PARTITION BY LINEAR HASH` 与 `PARTITION BY HASH` 效果等同。例子中 `PARTITION 4` 参数是无意义的。分区实际是按 SequoiaDB 的规则自动切分到对应的数据组中。

##KEY 分区##

- **KEY(\<column_list\>)**

    该分区方式根据指定的列计算 hash 值，利用 hash 值进行分区从而均匀打散记录，可以指定一个列或多个列，不限制列的类型。以下示例是根据货物 id 进行分区：

    ```lang-sql
    MariaDB [company]> CREATE TABLE goods (
        id INT NOT NULL,
        produced_date DATE,
        name VARCHAR(100),
        company VARCHAR(100)
    )
    PARTITION BY KEY (id)
    PARTITIONS 4;
    ```

    由于 SequoiaDB 引擎有独立的 hash 算法，`PARTITION BY LINEAR KEY` 与 `PARTITION BY KEY` 效果等同，均是使用 SequoiaDB 的算法。而例子中 `PARTITION 4` 参数是无意义的。分区实际是按 SequoiaDB 的规则自动切分到对应的数据组中。

 上述示例与以下语句对应：

    ```lang-sql
    MariaDB [company]> CREATE TABLE goods (
        id INT NOT NULL,
        produced_date DATE,
        name VARCHAR(100),
        company VARCHAR(100)
    ) 
    COMMENT='sequoiadb:{ table_options: { ShardingKey: { id: 1 }, ShardingType: "hash", AutoSplit: true } }'
    ```

##复合分区##

复合分区中，上层分区必须使用 RANGE 或者 LIST 分区，下层分区必须使用 HASH 或者 KEY 分区。以下示例是在 goods 表上先根据 produced_date 进行 RANGE 分区，再使用每个范围分区的 id 进行 HASH 分区。

```lang-sql
MariaDB [company]> CREATE TABLE goods (
    id INT NOT NULL,
    produced_date DATE,
    name VARCHAR(100),
    company VARCHAR(100)
)
PARTITION BY RANGE COLUMNS (produced_date)
SUBPARTITION BY KEY (id)
SUBPARTITIONS 2 (
    PARTITION p0 VALUES LESS THAN ('1990-01-01'),
    PARTITION p1 VALUES LESS THAN ('2000-01-01'),
    PARTITION p2 VALUES LESS THAN ('2010-01-01'),
    PARTITION p3 VALUES LESS THAN ('2020-01-01')
);
```

##注意事项##

* 不支持指定特定的 HASH 分区操作

* 不支持从 INFORMATION_SCHEMA.PARTITIONS 表查询 HASH 分区后各个分区具体记录数

* 不支持使用自增字段作为 LIST/RANGE 的分区字段

* 不支持 EXCHANGE PARTITION 操作

* RANGE COLUMNS 有多个列时，不能指定分区操作
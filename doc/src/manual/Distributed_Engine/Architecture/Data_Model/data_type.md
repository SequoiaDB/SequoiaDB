[^_^]:
    概述
    作者：谭钊波
    时间：20190701
    评审意见
    王涛：
    许建辉：
    市场部：20190918


SequoiaDB 巨杉数据库 JSON 实例支持的数据类型如下表所示：

| 值类型 | 定义 | 比较优先级权值 | 用例 |
| -------- | ---- | -------------- | ---- |
| 整数 | 范围：-2147483648~2147483647 | 10 | ```{ "key" : 123 }``` |
| 长整数 | 范围：-9223372036854775808~9223372036854775807<br>如果用户指定的数值无法适用于整数，则 SequoiaDB 自动将其转化为浮点型 | 10 | ```{ "key" : 3000000000 }``` 或<br>```{ "key" : { "$numberLong" : "3000000000" } }``` |
| 浮点数 | 范围：-1.7E+308~1.7E+308 | 10 | ```{ "key" : 123.456 }``` 或<br>```{ "key" : 123e+50 }``` |
| [高精度数][data_mode_data_type_decimal] | 范围：小数点前最高 131072 位，小数点后最高 16383 位  | 10 | ```{ "key" : { $decimal:"123.456" } }``` |
| 字符串 | 双引号包含的字符串 | 15 | ```{ "key" : "value" }``` |
| [对象 ID(OID)][data_mode_data_type_oid] | 由十二字节字符构建而成 | 35 | ```{ "key" : { "$oid" : "123abcd00ef12358902300ef" } }``` |
| 布尔 | true 或者 false | 40 | ```{ "key" : true }``` 或 ```{ "key" : false }``` |
| [日期][data_mode_data_type_date] | 格式：YYYY-MM-DD<br>范围：0000-01-01~9999-12-31 | 45 | ```{ "key" : { "$date" : "2012-01-01" } }``` |
| [时间戳][data_mode_data_type_timestamp] | 格式：YYYY-MM-DD-HH.mm.ss.ffffff<br>范围：1902-01-01 00:00:00.000000~2037-12-31 23:59:59.999999 | 45 | ```{ "key" : { "$timestamp" : "2012-01-01-13.14.26.124233" } }``` |
| [二进制数据][data_mode_data_type_binary] | Base64 形式的二进制数据 | 30 | ```{ "key" : { "$binary" : "aGVsbG8gd29ybGQ=", "$type" : "1" } }``` |
| [正则表达式][data_mode_data_type_regex] | 正则表达式 | 50 | ```{ "key" : { "$regex" : "^张", "$options" : "i" } }``` |
| [对象][data_mode_data_type_object] | 嵌套 JSON 文档 | 20 | ```{ "key" : { "subobj" : "value" } }``` |
| [数组][data_mode_data_type_array] | 嵌套数组对象 | 25 | ```{ "key" : [ "abc", 0, "def" ] }``` |
| 空值 | null | 5 | ```{ "key" : null }``` |
| 最小值 | 比所有值小 | -1 | ```{ "key" : {"$minKey": 1 } }``` |
| 最大值 | 比所有值大 | 127 | ```{ "key" : {"$maxKey": 1 } }``` |

> **Note:**
>
> - 不同类型字段的值进行比较时，比较优先级权值越大，该类型的值就越大。
> - 日期和时间戳支持夏令时。

高精度数类型
----

SequoiaDB 提供的高精度数可以满足银行等企业存储及运算高精度数据的需求。

### 取值范围 ###

高精度数可确保小数点前最高 131072 位及小数点后最高 16383 位数字的精度。

### 格式 ###

高精度数两种表达形式如下：

```lang-json
方式一：{ <fieldName> : { "$decimal" : "<数据>" } }
方式二：{ <fieldName> : { "$decimal" : "<数据>", "$precision" : [<总精度>, <小数精度>] } }
```

其中 fieldName 为数据的字段名；“数据”为字符串形式的数字；“总精度”为该数据整数部分及小数部分的精度范围的总和；“小数精度”为该数据小数部分的精度范围。

通过方式一定义的高精度数并没有指定精度范围，在这种情况下，数据默认使用最大的精度范围存储。通过方式二定义的高精度数同时指定了总精度和小数精度，数据将按照该精度要求存储。

由于使用高精度数需要确保精度不丢失，所以数据会占用更多的存储空间及花费更高的运算代价。

> **Note:**
>
> 高精度数的更多用法可参考 [NumberDecimal][data_type_numberDecimal]。

### 示例 ###

* 使用高精度数表示 double 类型无法存储的数字：1.88888E+308

   ```lang-json
   { "num" : { "$decimal" : "1.88888E+308" } }
   ```

* 指定数据的精度

   ```lang-json
   { "pi" : { "$decimal" : "3.14179526", "$precision" : [ 20, 18 ] } }
   ```

对象ID类型
----

对象 ID 又称为 OID，由 12 字节 16 进制字符构造而成。

### 取值范围 ###

12 字节的字符包含如下内容：

* 4 字节精确到秒的时间戳
* 3 字节系统（物理机）标示
* 2 字节线程 ID
* 3 字节由随机数起始的序列号

| 4 字节时间戳 | 3 字节系统标示 | 2 字节线程ID | 3 字节序列号 |
| ------------ | -------------- | ------------ | ------------ |

一台机器的一个线程在一秒钟内最多可以生成 16777216 个不同的 OID 数值，因此基本可以认为在集群环境中，所有的 OID 数值都是全局唯一的。

在 SequoiaDB 中，每个集合存放的文档必须拥有一个 _id 字段。该字段的值类型为 OID 类型，并且该字段值在集合中唯一。

### 格式 ###

对象 ID 的表达形式如下：

```lang-json
{ <fieldName> : { "$oid" : "<12字节16进制字符>" } }
```

> **Note:**
>
> 对象 ID 的更多用法可参考 [OID][data_type_OID]。

### 示例 ###

由 SequoiaDB 生成的对象 ID 的显示如下：

```lang-json
{ "_id" : { "$oid" : "5d1eea4d7e9eb6328c0c463e" } }
```

其中前 4 字节 16 进制字符"5d1eea4d"表示的时间戳为 1562307149，即为：2019-07-05 14:12:29；第 5 到第 7 字节 16 进制字符"7e9eb6"表示的机器标识为 8298166；第 8 到第 9 字节 16 进制字符"328c"表示的线程号为 12940；最后 3 字节 16 进制字符"0c463e"表示随机数 804414。

日期类型
----

SequoiaDB 中的日期使用 YYYY-MM-DD 的形式存取，在存储时将其转换为 8 字节的整数。

### 取值范围 ###

日期类型的取值范围为：0000-01-01~9999-12-31。

### 格式 ###

日期的表达形式如下：

```lang-json
{ <fieldName> : { "$date" : "<YYYY-MM-DD>" } }
```

> **Note:**
>
> 日期类型的更多用法可参考 [SdbDate][data_type_date]。

### 示例 ###

```lang-json
{ "createTime" : { "$date" : "2012-05-12" } }
```

时间戳类型
----

SequoiaDB 中的时间戳使用 YYYY-MM-DD-HH.mm.ss.ffffff 的形式存取，在存储时将其转换为 8 字节的整数。

### 取值范围 ###

时间戳类型的取值范围为：1902-01-01 00:00:00.000000~2037-12-31 23:59:59.999999。

### 格式 ###

时间戳的表达形式如下：

```lang-json
{ <fieldName> : { "$timestamp" : "<YYYY-MM-DD-HH.mm.ss.ffffff>" } }
```

> **Note:**
>
> 时间戳类型的更多用法可参考 [Timestamp][data_type_timestamp]。

### 示例 ###

```lang-json
{ "createTime" : { "$timestamp" : "2012-05-12-13.15.21.241523" } }
```

二进制类型
----

SequoiaDB 可以存储一定长度的二进制类型的数据。

### 取值范围 ###

SequoiaDB 存储二进制类型的数据时，要求用户先使用 Base64 方式对二进制内容进行编码，然后将编码后的字符串存放到数据库。

### 格式 ###

二进制数据的表达形式如下：

```lang-json
{ <fieldName> : { "$binary" : "<数据>", "$type" : <类型> } }
```

其中“数据”必须为 Base64 编码后的字符串，“类型”为 0~255 之间的十进制数值。用户可以任意指定该范围之间的数值作为应用程序中的类型标识。

Base64 为一种通用的数据转换形式，主要将二进制数据转化为以纯 ASCII 字符串表示的字节流。一般来说转换之后的数据长度会大于原本的数据长度。

为了节省空间，在 SequoiaDB 的内部存放数据时，会将 Base64 编码后的数据解码为原始数据进行存放。当用户读取数据时会再次将其转化为 Base64 形式发送。

> **Note:**
>
> - BSON 最大能够装载 16MB 内容。当二进制内容太大时，可以考虑使用 [Lob][data_mode_lob] 来存放二进制内容。
> - 二进制数据类型的更多用法可参考 [BinData][data_type_binary]。

### 示例 ###

字符串"hello world"被 Base64 编码后的数据为"aGVsbG8gd29ybGQ="。包含"hello world"二进制数据，且类型为 1 的 JSON 数据为：

```lang-json
{ "binary_data" : { "$binary" : "aGVsbG8gd29ybGQ=", "$type" : "1" } }
```

正则表达式类型
----

SequoiaDB 可以使用正则表达式检索用户数据。

### 取值范围 ###

正则表达式规则，可参阅 [Perl 正则表达式手册][data_mode_web_regex]。

### 格式 ###

正则表达式输入的格式如下：

```lang-json
{ <fieldName> : { "$regex" : "正则表达式", "$options" : "选项" } }
```

其中“正则表达式”为一个正则表达式字符串，“选项”则参见下表：

| 选项 | 描述 |
| ---- | ---- |
| i    | 匹配时不区分大小写 |
| m    | 允许进行多行匹配；当该参数打开时，字符“\^”与“\&”匹配换行符之后的与之前的字符 |
| x    | 忽略正则表达式匹配中的空白字符；如果需要使用空白字符，在空白字符之前使用反斜线“\\”进行转意 |
| s    | 允许“.”字符匹配换行符|

当使用选项时，用户可以任意组合使用这些选项。

> **Note:**
>
> 正则表达式类型的更多用法可参考 [Regex][data_type_regex]。

### 示例 ###

使用正则表达式进行大小写忽略，匹配以字符“W”起始的字符串，可以使用：

```lang-json
{ "regex_data" : { "$regex" : "^W", "$options" : "i" } }
```

对象类型
----

在 SequoiaDB 的记录中，用户可以使用对象类型来表示嵌套结构。

### 取值范围 ###

对象类型表示的嵌套结构可以包含一个或者多个任意的字段。

### 格式 ###

对象由"{"（左大括号）起始，至 "}"（右大括号）结束，其中包含至少一个键值对。

```lang-json
{ "fieldName" : { <字段1>, <字段2>, <字段n> } }
```

对象中各字段并没有固定的顺序，当对字段进行操作（如更新操作）时，字段之间的顺序可能会被调换。当表示嵌套对象中的某一个字段时，可以使用 "." (句号)在字段名之间分割。

### 示例 ###

在记录中使用嵌套结构，例如：

```lang-json
{ "address" : { "city" : "Guangzhou", "street" : "Beijing Road" } }
```

用户可以使用 "address.city" 代表嵌套对象中的字段名 "city"。

数组类型
----

在 SequoiaDB 的记录中，当某一字段对应多个值时，用户可以使用数组结构存放数据。

### 取值范围 ###

数组中的值可以为同一种类型，也可以为不同类型。值的数据类型可以为 JSON 支持的任意数据类型（包括嵌套对象和数组）。

### 格式 ###

数组由“[”（左中括号）起始，至“]”（右中括号）结束，其中包含零个或多个值。

```lang-json
{ "fieldName" : [ <值0>, <值1>, <值n> ] }
```

数组中的值是有序的，每个值都存在下标，且下标从 0 开始递增。在进行数据操作过程，数组中值的下标不会改变。在驱动中，用户可以通过下标的字符串来存取元素的值（即：下标的字符串为值对应的键）。

当访问数组中某个元素时，可以通过“字段名.下标”的方式来访问该元素。

### 示例 ###

在数组中存放多种数据类型的元素，例如：

```lang-json
{ "array_data" : [ "hello", "world", 1, { "a" : 1, "b" : 2} ] }
```

其中"hello"在数组中的下标为 0，而"world"在数组中的下标为 1。如果希望表示 array_data 中"world"这个值时，可以使用"array_data.1"作为字段名。

JSON实例与SQL实例数据类型映射关系
----

SequoiaDB 巨杉数据库采用存储与计算分离的模型。SequoiaDB 是数据存储层，存储真实的业务数据；而 SQL 实例（如：MySQL 实例、PostgreSQL 实例和SparkSQL 实例等）为计算层。计算层的 SQL 实例只存储如 DDL 等元数据，而不会存储任何的业务数据。在数据类型层面，存储层的 JSON 实例与计算层的 SQL 实例存在以下映射关系：

|类型 | JSON 实例 | MySQL 实例 | PostgreSQL 实例 | SparkSQL 实例 |
| --- | ---       | ---        | ---             | ---           |
| 整数       | int32     | INT/BIT/<br>TINYINT/SMALLINT | smallint/integer/serial | int |
| 长整数     | int64     | INT/BIT/BIGINT | bigint/bigserial          | bigint |
| 浮点数     | double    | FLOAT/DOUBLE   | real/double precision     | double |
| 高精度数   | decimal   | DECIMAL        | numeric/decimal           | decimal |
| 字符串     | string    | CHAR/VARCHAR/<br>TINYTEXT/TEXT/<br>MEDIUMTEXT/<br>LONGTEXT | text/char/varchar | string |
| 对象 ID    | objectId  | NA             | NA                        | string |
| 布尔       | boolean   | NA             | boolean                   | boolean |
| 日期       | date      | DATE           | date                      | date |
| 时间戳     | timestamp | TIMESTAMP      | timestamp                 | timestamp |
| 二进制数据 | binary    | BINARY         | bytea                     | binary |
| 正则表达式 | regex     | NA             | NA                        | NA |
| 对象       | object    | NA             | NA                        | struct\<field:type,...\> |
| 数组       | array     | NA             | TYPE[]                    | array\<type\> |
| 空值       | null      | NA             | text                      | NA |
| 最小值     | max       | NA             | NA                        | NA |
| 最大值     | min       | NA             | NA                        | NA |

> **Note:**
>
>* 关于 MySQL 实例与 JSON 实例数据类型映射可参考 [MySQL 实例数据类型映射][data_mode_mysql_data_type_reference]。
>* 关于 PostgreSQL 实例与 JSON 实例数据类型映射可参考 [PostgreSQL 实例数据类型映射][data_mode_postgresql_data_type_reference]。
>* 关于 SparkSQL 实例与 JSON 实例数据类型映射可参考 [SparkSQL 实例数据类型映射][data_mode_sparksql_data_type_reference]。


其它参考
----

如果用户需要使用 Java 驱动构建 BSON 记录可参考 [Java 驱动构建 BSON][data_mode_java_build_bson]。


[^_^]:
    本文使用到的所有内部链接及引用：
[data_mode_data_type_decimal]:manual/Distributed_Engine/Architecture/Data_Model/data_type.md#高精度数类型
[data_mode_data_type_oid]:manual/Distributed_Engine/Architecture/Data_Model/data_type.md#对象ID类型
[data_mode_data_type_date]:manual/Distributed_Engine/Architecture/Data_Model/data_type.md#日期类型
[data_mode_data_type_timestamp]:manual/Distributed_Engine/Architecture/Data_Model/data_type.md#时间戳类型
[data_mode_data_type_binary]:manual/Distributed_Engine/Architecture/Data_Model/data_type.md#二进制类型
[data_mode_data_type_regex]:manual/Distributed_Engine/Architecture/Data_Model/data_type.md#正则表达式类型
[data_mode_data_type_object]:manual/Distributed_Engine/Architecture/Data_Model/data_type.md#对象类型
[data_mode_data_type_array]:manual/Distributed_Engine/Architecture/Data_Model/data_type.md#数组类型

[data_mode_lob]:manual/Distributed_Engine/Architecture/Data_Model/lob.md

[data_type_numberDecimal]:manual/Manual/Sequoiadb_Command/SpecialObjects/NumberDecimal.md
[data_type_OID]:manual/Manual/Sequoiadb_Command/SpecialObjects/OID.md
[data_type_date]:manual/Manual/Sequoiadb_Command/SpecialObjects/SdbDate.md
[data_type_timestamp]:manual/Manual/Sequoiadb_Command/SpecialObjects/Timestamp.md
[data_type_binary]:manual/Manual/Sequoiadb_Command/SpecialObjects/BinData.md
[data_type_regex]:manual/Manual/Sequoiadb_Command/SpecialObjects/Regex.md

[data_mode_mysql_data_type_reference]:manual/Database_Instance/Relational_Instance/MySQL_Instance/data_type.md
[data_mode_postgresql_data_type_reference]:manual/Database_Instance/Relational_Instance/PostgreSQL_Instance/data_type.md
[data_mode_sparksql_data_type_reference]:manual/Database_Instance/Relational_Instance/SparkSQL_Instance/Operation/usage.md
[data_mode_java_build_bson]:manual/Database_Instance/Json_Instance/Development/java_driver/java_bson_usage.md

[data_mode_web_regex]:http://perldoc.perl.org/perlre.html


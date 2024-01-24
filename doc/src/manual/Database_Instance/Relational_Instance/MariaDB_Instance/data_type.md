
MariaDB 支持多种 SQL 数据类型：数值类型、date 类型、time 类型和字符串类型等。

从 MariaDB 实例到 SequoiaDB 巨杉数据库的 JSON 对象实例之间的数据类型映射关系如下：

| MariaDB 实例 | JSON 对象实例| 备注                                       |
| ---------- | ------------ | -------------------------------------------- |
| BIT        | int32/int64  | 超出 int32 范围则按 int64 存储               |
| BOOL       | int32        |                                              |
| TINYINT    | int32        |                                              |
| SMALLINT   | int32        |                                              |
| MEDIUMINT  | int32        |                                              |
| INT        | int32/int64  | 超出 int32 范围则按 int64 存储               |
| BIGINT     | int64/decimal | 超出 Long 范围则按 Decimal 存储             |
| FLOAT      | double       |                                              |
| DOUBLE     | double       |                                              |
| DECIMAL    | decimal      |                                              |
| YEAR       | int32        |                                              |
| DATE       | date         |                                              |
| TIME       | decimal      | 'HHMMSS[.fraction]'格式的 Decimal 值         |
| DATETIME   | string       | 'YYYY-MM-DD HH:MM:SS[.fraction]'格式的字符串 |
| TIMESTAMP  | timestamp    |                                              |
| CHAR       | string       |                                              |
| VARCHAR    | string       |                                              |
| BINARY     | binary       |                                              |
| VARBINARY  | binary       |                                              |
| TINYBLOB   | binary       |                                              |
| BLOB       | binary       |                                              |
| MEDIUMBLOB | binary       |                                              |
| LONGBLOB   | binary       | 最大长度 16MB                                |
| TINYTEXT   | string       |                                              |
| TEXT       | string       |                                              |
| MEDIUMTEXT | string       |                                              |
| LONGTEXT   | string       | 最大长度 16MB                                |
| ENUM       | int64        |                                              |
| SET        | int64        |                                              |
| JSON       | string       |                                              |
| GEOMETRY   | binary       | 不支持索引（spatial/non-spatial 索引）       |
| NULL       | -            | 不存储                                       |

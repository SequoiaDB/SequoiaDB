[^_^]:
    PostgreSQL 实例-数据类型映射表

PostgreSQL 支持多种 SQL 数据类型：数值类型、date 类型、字符串类型等。

PostgreSQL 实例与 SequoiaDB 巨杉数据库的数据类型映射关系如下：

| PostgreSQL	    |     SequoiaDB    | 备注                                      |
| ----------------- | ---------------- | --------------------------------------------- |
| smallint	        | int32            | 当类型为 int32 的值长度超过 smallint 的长度范围时，会发生截断 |
| integer        	| int32              |                                               |
| bigint        	| int64             |                                               |
| serial           	| int32              |                                               |
| bigserial      	| int64             |                                               |
| real              | double           | 存在精度问题，SequoiaDB 存储时不是完全一致    |
| double precision  | double           |                                               |
| numeric           | decimal/string | 在创建外表时，指定选项 decimal 为'on', numeric 映射对应 decimal ，否则对应 string   |
| decimal           | decimal/string | 在创建外表时，指定选项 decimal 为'on', decimal 映射对应 decimal ，否则对应 string   |
| text              | string           |                                               |
| char              | string           |                                               |
| varchar           | string           |                                               |
| bytea             | binary(type=0)   |                                               |
| date              | date             |                                               |
| timestamp         | timestamp        |                                               |
| TYPE[]            | array            | 仅支持一维数组                                |
| boolean           | boolean          |                                               |
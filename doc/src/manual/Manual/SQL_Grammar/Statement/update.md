
update 用于修改集合中的记录。

##语法##
***update \<cs_name\>.\<cl_name\> set (\<field1_name\>=\<value1\>,...) [where \<condition\>] [/*+\<hint1\> \<hint2\> ...*/]***

##参数##
| 参数名 | 参数类型 | 描述 | 是否必填 |
|--------|----------|------|----------|
| cs_name | string | 集合空间名  | 是 |
| cl_name | string | 集合空间名  | 是 |
| field1_name | string | 字段名  | 是 |
| value1 | string | 字段值  | 是 |
| condition | expression | 条件，只对符合条件的记录更新  | 是 |
| hint1 | [hint][hint1] | 指定执行方式  | 是 |

##返回值##
无 

##示例##

   * 集合 sample.employee 中原始记录如下：

   ```lang-json
   { age: 1 }
   { age: 2 }
   ```

   * 修改集合中全部的记录，将记录中的 age 字段值修改为20 

   ```lang-javascript
   > db.execUpdate( "update sample.employee set age=20" )
   ```

   * 修改符合条件的记录，只对符合条件 age \< 25 的记录做修改操作 

   ```lang-javascript
   > db.execUpdate( "update sample.employee set age=30 where age < 25" )
   ```

   * 指定更新时保留分区键，切分表 sample.employee 落在两个复制组上，分区键为 { b: 1 }

   ```lang-javascript
   > db.execUpdate( "update sample.employee set b = 1 where age < 25 /*+use_flag(SQL_UPDATE_KEEP_SHARDINGKEY)*/" )
   (nofile):0 uncaught exception: -178
   Sharding key cannot be updated
   ```


[^_^]:
    本文使用的所有引用及链接
[hint1]:manual/Manual/SQL_Grammar/Clause/hint.md
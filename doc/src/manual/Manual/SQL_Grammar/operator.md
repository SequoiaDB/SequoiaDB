
SequoiaDB 巨杉数据库内置 SQL 支持算术运算符、比较运算符和逻辑运算符。

## 算术运算符 ##

| 运算符  | 描述           |
|---------| -------------- |
| +       | 加 |
| -       | 减 |
| *       | 乘 |
| /       | 除 |
| %       | 模 |

>**Note:** 
>
> -   除法、取模运算，被除数为零时返回结果为 null。
> -   对非数值型做算术运算时返回的结果为 null。

## 逻辑运算符 ##

| 运算符  | 描述           |
|---------| -------------- |
| AND     | 与 |
| OR      | 或 |
| NOT     | 非 |

## 比较运算符 ##

| 运算符  | 描述           |
|---------| -------------- |
| >       | 大于 |
| <       | 小于 |
| >=      | 大于等于 |
| <=      | 小于等于 |
| =       | 等于 |
| <>      | 不等于 |
| is null | 字段不存在或者为 null |
| is not null | 字段存在且不为 null |

## 示例##

   * 集合 sample.employee 中原始记录如下：

   ```lang-json
   { a: 1 }
   { a: null }
   { b: 1 }
   ```

   * 查询 a 字段值为 null，或者不存在的记录 

   ```lang-javascript
    > db.exec('select * from sample.employee where a is null')
    {
      "_id": {
        "$oid": "599547f22d8380a914000000"
    },
      "b": 1
    }
    {
      "_id": {
        "$oid": "599548262d8380a914000002"
    },
      "a": null
    }
   Return 2 row(s).
   ```

   * 查询 a 字段值存在且不为 null 的记录 

   ```lang-javascript
   > db.exec('select * from sample.employee where a is not null')
    {
      "_id": {
        "$oid": "598d0b57a6e2e2fd65000000"
      },
      "a": 1
    }
    Return 1 row(s).
   ```

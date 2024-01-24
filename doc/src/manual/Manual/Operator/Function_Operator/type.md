
##语法##

```lang-json
{ <字段名>: { $type: <1|2> } }
```

##说明##

返回字段值的类型

| 值  | 描述 | 例子 |
| --- | ---- | ---- |
| 1   | 表示获取数值形式结果 | 16 |
| 2   | 表示获取字符串形式的结果 | int32 |

##类型列表##

| Type   | 描述   | 数值形式   | 字符串形式 |
| ------ | ------ | ------ | ------ |
| 32-bit integer | 整型，范围-2147483648至2147483647 | 16 | int32 |
| 64-bit integer | 长整型，范围-9223372036854775808至9223372036854775807。如果用户指定的数值无法适用于整数，则 SequoiaDB 自动将其转化为长整数。 | 18 | int64 |
| double | 浮点数，范围-1.7E+308至1.7E+308 | 1 | double |
| decimal | 高精度数，范围小数点前最高131072位;小数点后最高16383位 | 100 | decimal |
| string | 字符串 | 2 | string |
| ObjectID | 十二字节对象 ID | 7 | oid |
| boolean | 布尔（true 或 false） | 8 | bool |
| date | 日期（YYYY-MM-DD） | 9 | date ||
| timestamp | 时间戳（YYYY-MM-DD-HH.mm.ss.ffffff） | 17 | timestamp |
| Binary data | Base64 形式的二进制数据 | 5 | bindata |
| Regular expression | 正则表达式 | 11 | regex |
| Object | 嵌套 JSON 文档对象 | 3 | object |
| Array | 嵌套数组对象 | 4 | array |
| null | 空 | 10 | null |
| MinKey | 最小值 | -1 | minkey |
| MaxKey | 最大值 | 127 | maxkey |

##示例##

在集合 sample.employee 插入如下记录：

```lang-javascript 
> db.sample.employee.insert( { "a" : 123 } )
> db.sample.employee.insert( { "a": "abc" } )
```

* 作为选择符使用，返回字段 a 值的类型

  ```lang-javascript
  > db.sample.employee.find( {}, { "a": { "$type": 1 } } )
  {
      "_id": {
        "$oid": "5832623892a95ad71f000000"
      },
      "a": 16
  }
  {
      "_id": {
        "$oid": "5832624692a95ad71f000001"
      },
      "a": 2
  }
  Return 2 row(s).
  ```

  > **Note:**  
  > { "a": 123 } 中，a 为整数类型，类型数值为 16。  
  > { "a": "abc" } 中，a 为字段串类型，类型数值为 2。

  ```lang-javascript
  > db.sample.employee.find( {}, { "a": { "$type": 2 } } )
  {
      "_id": {
        "$oid": "5832623892a95ad71f000000"
      },
      "a": "int32"
  }
  {
      "_id": {
        "$oid": "5832624692a95ad71f000001"
      },
      "a": "string"
  }
  Return 2 row(s).
  ```

  > **Note:**  
  > { "a": 123 } 中，a 为字段串类型，类型字符串值为“int32”。  
  > { "a": "abc" } 中，a 为字段串类型，类型字符串值为“string”。

* 与匹配符配合使用，匹配字段 a 值的类型为字符串类型的记录
  
  ```lang-javascript
  > db.sample.employee.find( { "a": { "$type": 1, "$et": 2 } } )
  {
      "_id": {
        "$oid": "5832624692a95ad71f000001"
      },
      "a": "abc "
  }
  Return 1 row(s).
  ```

* 与匹配符配合使用，匹配字段 a 值的类型为整数类型的记录

  ```lang-javascript
  > db.sample.employee.find( { "a": { "$type": 2, "$et": "int32" } } )
  {
      "_id": {
        "$oid": "5832623892a95ad71f000000"
      },
      "a": 123
  }
  Return 1 row(s).
  ```

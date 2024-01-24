
##语法##

```lang-json
{ <字段名>: { $cast: <目标类型> } }
```

##说明##
将字段名对应的内容转化为目标类型的内容，当原始内容为数组时，对数组中每个元素执行该操作。可通过字符串（大小写不敏感）或数字的方式指定目标类型。
  
目标类型如下表所示：

| 目标类型  | 字符串表示  | 数字表示 |
| --------- | ----------- | -------- |
| MinKey    | "minkey"    | -1       |
| Double    | "double"    | 1        |
| String    | "string"    | 2        |
| Object    | "object"    | 3        |
| ObjectId  | "oid"       | 7        |
| Bool      | "bool"      | 8        |
| Date      | "date"      | 9        |
| Null      | "null"      | 10       |
| Int32     | "int32"     | 16       |
| Timestamp | "timestamp" | 17       |
| Int64     | "int64"     | 18       |
| Decimal   | "decimal"   | 100      |
| MaxKey    | "maxkey"    | 127      |

转换关系如下：

* MinKey

  | 源类型   | 转换格式备注 | 异常返回 |
  | -------- | ------------ | -------- |
  | ALL      | 任何类型都能转换成 MinKey | 无 |

* Double

  | 源类型   | 转换格式备注 | 异常返回 |
  | -------- | ------------ | -------- |
  | String   | 将数字字符串转换为 Double 类型对应的数字 | 0.0 |
  | Bool     | true: 1.0；false: 0.0 | 无 |
  | Int32    | 将 Int32 类型的数字强转为 Double 类型对应的数字 | 无 |
  | Int64    | 将 Int64 类型的数字强转为 Double 类型对应的数字 | 无 |
  | Decimal  | 将 Decimal 类型的数字强转为 Double 类型对应的数字 | 0.0 |
  | Timestamp| 将 Timestamp 类型的值表示的绝对毫秒转为 Double 类型的数字 | 无 |
  | Date     | 将 Date 类型的值表示的绝对毫秒转为 Double 类型的数字 | 无 |
  > **Note:**  
  > 1. 将非数字字符串转为 Double 类型，转换将异常返回 0.0。  
  > 2. 将超过 Double 类型范围的数字字符串转为 Double 类型，转换将异常返回 0.0。  
  > 3. 将 Int64/Decimal 类型的值转为 Double 类型时，可能会发生精度的丢失。  
  > 4. 将超过 Double 类型范围的 Decimal 类型的值转为 Double 类型，转换将异常返回 0.0。

* String

  | 源类型   | 转换格式备注 | 异常返回 |
  | -------- | ------------ | -------- |
  | Int32    | 将 Int32 类型的数字转换为字符串 | 无 |
  | Int64    | 将 Int64 类型的数字转换为字符串 | 无 |
  | Double   | 将 Double 类型的数字转换为字符串 | 无 |
  | Decimal  | 将 Decimal 类型的数字转换为字符串 | 无 |
  | Date     | 将 Date 类型的内容转换为字符串 | 无 |
  | Timestamp| 将 Timestamp 类型的内容转换为字符串 | 无 |
  | ObjectId | 将 ObjectId 类型的内容转换为字符串 | 无 |
  | Object   | 将 Object 类型的内容转换为Json格式的字符串 | 无 |
  | Array    | 将数组中每个元素都转换为对应内容的字符串 | 无 |
  | Bool     | 将 Bool 类型的内容转换为字符串 | 无 |

* Object

  | 源类型   | 转换格式备注 | 异常返回 |
  | -------- | ------------ | -------- |
  | String   | 将标准 Json 字符串转为 Object 类型 | null     |
  > **Note:**  
  >
  > 当字符串为非标准 Json 格式时，转换将异常返回 null。  

* ObjectId

  | 源类型   | 转换格式备注 | 异常返回 |
  | -------- | ------------ | -------- |
  | String   | 将由 24 个 16 进制字符组成的字符串（带字符串结束符共 25 个字符）转为 ObjectId 类型 | null |
  > **Note:**  
  >
  > 当字符串表示的内容有误，转换将异常返回 null。

* Bool

  | 源类型   | 转换格式备注 | 异常返回 |
  | -------- | ------------ | -------- |
  | Int32    | Int32 类型的数值为 0 表示 false，其他表示 true | 无 |
  | Int64    | Int64 类型的数值为 0 表示 false，其他表示true | 无 |
  | Double   | Double 类型的数值为 0 表示 false，其他表示 true | 无 |
  | Decimal  | Decimal 类型的数值为 0 表示 false，其他表示 true | 无 |

* Date

  | 源类型   | 转换格式备注 | 异常返回 |
  | -------- | ------------ | -------- |
  | Int32    | Int32 类型的数字理解为绝对秒数，将绝对秒数转换为 Date 类型对应的时间 | 无 |
  | Int64    | Int64 类型的数字理解为绝对毫秒，将绝对毫秒转换为 Date 类型对应的时间 | 无 |
  | Double   | Double 类型的数字理解为绝对毫秒，将绝对毫秒转换为 Date 类型对应的时间 | 无 |
  | Decimal  | Decimal 类型的数字理解为绝对毫秒，将绝对毫秒转换为 Date 类型对应的时间 | null |
  | String   | 将形如“2015-08-19”格式的字符串转换为 Date 类型对应的时间，支持范围为["0000-01-01","9999-12-31"] | null |
  | Timestamp| 将Timestamp类型的日期部分转换为Date类型对应的时间 | 无 |
  > **Note:**  
  >
  > - 绝对秒数：距离格林威治时间1970年01月01日00时00分00秒的总秒数。当 Int32 类型的值为负数时，转换结果为该时间点之前的 Date 类型的值，如：1969-12-31。  
  > - 绝对毫秒：距离格林威治时间1970年01月01日00时00分00秒的总毫秒数。当 Int64 类型的值为负数时，转换结果为该时间点之前的 Date 类型的值，如：1969-12-31。  
  > - 将 Double/Decimal 类型的转为 Date 类型，可能会发生精度丢失。因为转换过程是先由 Double/Decimal 类型转为 Int64 类型（此处可能发生精度丢失），然后再将 Int64 类型表示的绝对毫秒转换为 Date 类型对应的时间。  
  > - 当 Decimal 类型的值超出 Int64 类型范围时，转换将异常返回 null。  
  > - 将不在["0000-01-01","9999-12-31"]范围的字符串日期转换为 Date 类型，转换将异常返回 null。

* Null

  | 源类型   | 转换格式备注 | 异常返回 |
  | -------- | ------------ | -------- |
  | ALL      | 任何类型都能转换成 Null | 无 |

* Int32

  | 源类型   | 转换格式备注 | 异常返回 |
  | -------- | ------------ | -------- |
  | String   | 将数字字符串转换为 In32 类型对应的数字 | 0 |
  | Bool     | true:1；false:0 | 无 |
  | Int64    | 将 Int64  类型的值转为 Int32 对应的数字 | 0 |
  | Double   | 将 Double 类型的值转为 Int32 对应的数字 | 0 |
  | Decimal  | 将 Decimal 类型的值转为 Int32 对应的数字 | 0 |
  | Timestamp| 将 Timestamp 类型的值表示的绝对秒数转为 Int32 类型对应的数字 | 0 |
  | Date     | 将 Date 类型的值表示的绝对秒数转为 Int32 类型对应的数字 | 0 |
  > **Note:**  
  >
  > - 将非数字类型的字符串转为 Int32 类型，转换将异常返回 0。  
  > - 将超过 Int32 类型范围的数字字符串转为 Int32 类型，转换将异常返回 0。  
  > - 当 Int64/Double/Decimal 类型的值超出 Int32 类型范围时，转换将异常返回 0。  
  > - 当 Timestamp/Date 类型的值表示的绝对秒数超出 Int32 类型范围时，转换将异常返回 0。

* Timestamp

  | 源类型   | 转换格式备注 | 异常返回 |
  | -------- | ------------ | -------- |
  | Int32    | Int32 类型的数字理解为绝对秒数，将绝对秒数转换为 Timestamp 类型对应的时间 | 无 |
  | Int64    | Int64 类型的数字理解为绝对毫秒，将绝对毫秒转换为 Timestamp 类型对应的时间 | null |
  | Double   | Double 类型的数字理解为绝对毫秒，将 Double 类型的值强转为 Int64 类型的值，再将绝对毫秒转换为 Timestamp 类型对应的时间 | null |
  | Decimal  | Double 类型的数字理解为绝对毫秒，将 Decimal 类型的值强转为 Int64 类型的值，再将绝对毫秒转换为 Timestamp 类型对应的时间 | null |
  | String   | 将形如“2015-08-19-17.59.05.918488”格式的字符串转换为 Timestamp 类型，支持范围为["1902-01-01 00:00:00.000000", "2037-12-31 23:59:59.999999"] | null |
  | Date     | 将 Date 类型的内容转换为 Timestamp 类型对应的时间 | null |
  > **Note:**  
  > - 由于 Int64/Double/Decimal 类型表示的绝对毫秒能表示的时间范围比 Timestamp 类型的广，在这些类型转为 Timestamp 类型过程，若转换结果超出 Timestamp 类型能表示的范围，转换将异常返回 null。  
  > - 由于 Date 类型能表示的时间范围比 Timestamp 类型的广，在 Date 类型转为 Timestamp 类型过程，若转换结果超出 Timestamp 类型能表示的范围，转换将异常返回 null。

* Int64

  | 源类型   | 转换格式备注 | 异常返回 |
  | -------- | ------------ | -------- |
  | String   | 将数字字符串转换为 In64 类型对应的数字 | 0 |
  | Bool     | true:1；false:0 | 无 |
  | Int32    | 将 Int32 类型的值转为 Int64 类型的数字 | 无 |
  | Double   | 将 Double 类型的值转为 Int64 类型的数字 | 无 |
  | Decimal  | 将 Decimal 类型的值转为 Int64 类型的数字 | 0 |
  | Timestamp| 将 Timestamp 类型的值表示的绝对毫秒转为 Int64 类型的数字 | 无 |
  | Date     | 将 Date 类型的值表示的绝对毫秒转为 Int64 类型的数字 | 无 |
  > - 将非数字字符串转为 Int64 类型，转换将异常返回 0。  
  > - 将超过 Int64 类型范围的数字字符串转为 Int64 类型，转换将异常返回 0。
  > - 当 Decimal 类型的值超出 Int64 类型范围时，转换将异常返回 0。

* Decimal

  | 源类型   | 转换格式备注 | 异常返回 |
  | -------- | ------------ | -------- |
  | String   | 将数字字符串转换为 Decimal 类型对应的数字 | 0 |
  | Bool     | true: 1, false: 0 | 无 |
  | Int32    | 将 Int32 类型的值转为 Decimal 类型对应的数字 | 无 |
  | Int64    | 将 Int64 类型的值转为 Decimal 类型对应的数字 | 无 |
  | Double   | 将 Double 类型的值转为 Decimal 类型对应的数字 | 无 |
  | Timestamp| 将 Timestamp 类型的值表示的绝对毫秒转为 Decimal 类型对应的数字 | 无 |
  | Date     | 将 Date 类型的值表示的绝对毫秒转为 Decimal 类型对应的数字 | 无 |
  > - 将非数字字符串转为 Decimal 类型，转换将异常返回 0。  
  > - 将超过 Decimal 类型范围的数字字符串转为 Decimal 类型，转换将异常返回 0。

* MaxKey

  | 源类型   | 转换格式备注 | 异常返回 |
  | -------- | ------------ | -------- |
  | ALL      | 任何类型都能转换成 MaxKey | 无 |

##示例##

集合 sample.employee 存在如下记录：

```lang-javascript 
{ "a": "123" }
```

* 单独作为选择符使用，返回字段 a 转换 int32 类型之后的记录如下：

  ```lang-javascript
  > db.sample.employee.find( {}, { "a": { "$cast": "int32" } } )
  {
      "_id": {
        "$oid": "582527402b4c38286d000019"
      },
      "a": 123
  }
  Return 1 row(s).
  ```
  该查询返回的结果为 int32 类型。

* 与匹配符配合使用，匹配字段 a 转换为 int32 之后值为“123”的记录：

  ```lang-javascript
  > db.sample.employee.find( { a: { $cast: "int32", $et: 123 } } )
  {
      "_id": {
        "$oid": "582527402b4c38286d000019"
      },
      "a": "123"
  }
  Return 1 row(s).
  ```
  该查询返回原始记录，字段 a 的值是字符串类型。






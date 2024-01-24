## 语法

```lang-json
{ $bit: { <字段名1>:{<options>:<值1>},<字段名2>:{<options>:<值2>}, ... } }
```

## 描述

$bit 操作符是将指定字段的值与指定的值进行位运算。如果原记录中不存在指定字段，则使用 0 作为字段的值与指定的值进行位运算。

options 选项：

| 格式 | 描述     |                 示例                          |
|------|----------|-----------------------------------------------|
| xor  | 与指定的值进行逻辑异或运算 | db.sample.employee.update({$bit:{a:{xor:5}}}) |
| or   | 与指定的值进行逻辑或运算 | db.sample.employee.update({$bit:{a:{or:5}}})  |
| and  | 与指定的值进行逻辑与运算 | db.sample.employee.update({$bit:{a:{and:5}}})        |
| not  | 将指定字段的值进行逻辑非运算，not 的取值不能为空，但仅作为占位符使用，无意义  | db.sample.employee.update({$bit:{a:{not:0}}})        |

## 示例

集合 sample.employee 存在如下记录：

```lang-json
{  "a": 5 }
```

* 执行逻辑异或运算：

 ```lang-javascript
 > db.sample.employee.update({$bit:{a:{xor:5}}})
 ```

 此操作后，记录更新为：

 ```lang-json
 { "a": 0 }
 ```

* 执行逻辑异或运算。由于原记录不存在 b 字段，所以使用 0 与 10 进行异或运算，并把结果赋予 b 字段：

 ```lang-javascript
 > db.sample.employee.update({$bit:{b:{xor:10}}})
 ```

 此操作后，记录更新为：

 ```lang-json
 { "a": 0,"b": 10 }
 ```
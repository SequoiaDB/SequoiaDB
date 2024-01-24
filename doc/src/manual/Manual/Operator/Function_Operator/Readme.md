
函数操作可以配合[匹配符][overview]和[选择符][Selector_Operator]使用，以实现更复杂的功能。

- 配合匹配符一起使用，可以对字段进行各种函数运算之后，再执行匹配操作。

  以下示例，匹配字段 a 长度为 3 的记录：

  ```lang-javascript
  > db.sample.employee.find({a:{$strlen:1, $et:3}})
  ```

  > **Note:** 
  >
  > 先获取字段 a 的长度，再用该长度与 3 比较，返回长度为 3 的记录。

- 作为选择符使用，可以对选取的字段进行函数运算，返回运算后的结果。

  以下示例，返回将字段 a 转大写的结果：

  ```lang-javascript
  > db.sample.employee.find({}, {a:{$upper:1}})
  ```

所有支持的函数操作如下：

| 函数                                                           | 描述             | 示例                                     |
| -------------------------------------------------------------- | ---------------- | ---------------------------------------- |
| [$abs][abs]            | 取绝对值         | db.sample.employee.find({}, {a:{$abs:1}}) |
| [$ceiling][ceiling]    | 向上取整         | db.sample.employee.find({}, {a:{$ceiling:1}}) |
| [$floor][floor]        | 向下取整         | db.sample.employee.find({}, {a:{$floor:1}}) |
| [$mod][mod]            | 取模运算         | db.sample.employee.find({}, {a:{$mod:1}}) |
| [$add][add]            | 加法运算         | db.sample.employee.find({}, {a:{$add:10}}) |
| [$subtract][subtract]  | 减法运算         | db.sample.employee.find({}, {a:{$subtract:10}}) |
| [$multiply][multiply]  | 乘法运算         | db.sample.employee.find({}, {a:{$multiply:10}}) |
| [$divide][divide]      | 除法运算         | db.sample.employee.find({}, {a:{$divide:10}}) |
| [$substr][substr]      | 截取子串         | db.sample.employee.find({}, {a:{$substr:[0,4]}}) |
| [$strlen][strlen]      | 获取指定字段的字节数 | db.sample.employee.find({}, {a:{$strlen:10}}) |
| [$strlenBytes][strlenBytes] | 获取指定字段的字节数 | db.sample.employee.find({}, {a:{$strlenBytes:10}}) |
| [$strlenCP][strlenCP]       | 获取指定字段的字符数 | db.sample.employee.find({}, {a:{$strlenCP:10}}) |
| [$lower][lower]        | 字符串转为小写   | db.sample.employee.find({}, {a:{$lower:1}}) |
| [$upper][upper]        | 字符串转为大写   | db.sample.employee.find({}, {a:{$upper:1}}) |
| [$ltrim][ltrim]        | 去除左侧空格     | db.sample.employee.find({}, {a:{$ltrim:1}}) |
| [$rtrim][rtrim]        | 去除右侧空格     | db.sample.employee.find({}, {a:{$rtrim:1}}) |
| [$trim][trim]          | 去除左右两侧空格 | db.sample.employee.find({}, {a:{$trim:1}}) |
| [$cast][cast]          | 转换字段类型     | db.sample.employee.find({}, {a:{$cast:"int32"}}) |
| [$size][size]          | 获取数组元素个数 | db.sample.employee.find({}, {a:{$size:1}}) |
| [$type][type]          | 获取字段类型     | db.sample.employee.find({}, {a:{$type:1}}) |
| [$slice][slice]        | 截取数组元素     | db.sample.employee.find({}, {a:{$slice:[0,2]}}) |

函数操作可以支持流水线式处理，多个函数流水线执行：

```lang-javascript
> db.sample.employee.find({a:{$trim:1, $upper:1, $et:"ABC"}})
```

> **Note:**
>
>先对字段 a 去除左右两侧空格，然后再转换成大写，最后匹配与"ABC"相等的记录

当字段类型为数组类型时，函数会对该字段做一次展开，并对每个数组元素执行函数操作。

以取绝对函值函数为例：

```lang-javascript
> db.sample.employee.find()
{
  "a": [
    1,
    -3,
    -9
  ]
}

> db.sample.employee.find({}, {a:{$abs:1}})
{
  "a": [
    1,
    3,
    9
  ]
}
```

[^_^]:
    本文使用的所有引用及链接
[overview]:manual/Manual/Operator/Match_Operator/Readme.md
[Selector_Operator]:manual/Manual/Operator/Selector_Operator/Readme.md
[abs]:manual/Manual/Operator/Function_Operator/abs.md
[ceiling]:manual/Manual/Operator/Function_Operator/ceiling.md
[floor]:manual/Manual/Operator/Function_Operator/floor.md
[mod]:manual/Manual/Operator/Function_Operator/mod.md
[add]:manual/Manual/Operator/Function_Operator/add.md
[subtract]:manual/Manual/Operator/Function_Operator/subtract.md
[multiply]:manual/Manual/Operator/Function_Operator/multiply.md
[divide]:manual/Manual/Operator/Function_Operator/divide.md
[substr]:manual/Manual/Operator/Function_Operator/substr.md
[strlen]:manual/Manual/Operator/Function_Operator/strlen.md
[lower]:manual/Manual/Operator/Function_Operator/lower.md
[upper]:manual/Manual/Operator/Function_Operator/upper.md
[ltrim]:manual/Manual/Operator/Function_Operator/ltrim.md
[rtrim]:manual/Manual/Operator/Function_Operator/rtrim.md
[trim]:manual/Manual/Operator/Function_Operator/trim.md
[cast]:manual/Manual/Operator/Function_Operator/cast.md
[size]:manual/Manual/Operator/Function_Operator/size.md
[type]:manual/Manual/Operator/Function_Operator/type.md
[slice]:manual/Manual/Operator/Function_Operator/slice.md
[strlenBytes]:manual/Manual/Operator/Function_Operator/strlenBytes.md
[strlenCP]:manual/Manual/Operator/Function_Operator/strlenCP.md

##语法##

```lang-json
{ <字段名>: { $substr: <值> } }
```

##说明##

返回字符串的子串，原始值为数组类型时对每个数组元素执行该操作，非字符串类型返回 null 。

##格式##

```lang-javascript
find({},{<fieldName>:{<$substr:[Pos,Len]>}})
```
* Pos 代表从字符串返回子串的起始下标，当Pos为负数时代表从字符串末尾倒数的位置。例如-2代表从字符串倒数第二个字符开始截取子串。
* Len 代表截取的长度。

```lang-javascript
find({},{<fieldName>:{<$substr:Value>}})
```
* 当 Value 大于等于0时，表示返回从下标 0 开始,长度为Value的子串。
* 当 Value 小于0时，代表返回从下标 Value 开始的子串，负数代表从字符串末尾倒数的位置。例如 -2 代表从字符串倒数第二个字符开始截取子串。

##示例##

在集合 sample.employee 插入如下记录：

```lang-javascript 
> db.sample.employee.insert( { "a": "abcdefg" } )
```

- **作为选择符使用**

  返回下标为 0，长度为 2 的子串

  ```lang-javascript
  > db.sample.employee.find( {}, { "a": { "$substr": 2 } } )
  {
      "_id": {
        "$oid": "58257afbec5c9b3b7e000002"
      },
      "a": "ab"
  }
  Return 1 row(s).
  ```

  返回倒数第二个字符开始的子串

  ```lang-javascript
  > db.sample.employee.find( {}, { "a": { "$substr": -2 } } )
  {
      "_id": {
        "$oid": "58257afbec5c9b3b7e000002"
      },
      "a": "fg"
  }
  Return 1 row(s).
  ```

  返回下标为 2，长度为 3 的子串

  ```lang-javascript
  > db.sample.employee.find( {}, { "a": { "$substr": [ 2, 3 ] } } )
  {
      "_id": {
        "$oid": "58257afbec5c9b3b7e000002"
      },
      "a": "cde"
  }
  Return 1 row(s).
  ```

  返回倒数第二个字符开始，长度为 3 的子串

  ```lang-javascript
  > db.sample.employee.find( {}, { "a": { "$substr": [ -2, 3 ] } } )
  {
      "_id": {
        "$oid": "58257afbec5c9b3b7e000002"
      },
      "a": "fg"
  }
  Return 1 row(s).
  ```

  > **Note:**  
  >
  > 子串长度不足3。

- **与匹配符配合使用**

  匹配字段 a 下标为 2，长度为 3 的子串为“cde”的记录

  ```lang-javascript
  > db.sample.employee.find( { "a": { "$substr": [ 2, 3 ], "$et": "cde" } } )
  {
      "_id": {
        "$oid": "58257afbec5c9b3b7e000002"
      },
      "a": "abcdefg"
  }
  Return 1 row(s).
  ```



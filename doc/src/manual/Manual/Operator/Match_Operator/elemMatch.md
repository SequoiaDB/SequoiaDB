
##语法##

```lang-json
{ <fieldName>: { $elemMatch: <cond> } }
```

##描述##

如果数组或者对象中至少有一个元素符合条件，则返回该条记录

> **Note:**
>
> $elemMatch 匹配条件暂不支持全文检索语法

##示例##

集合 sample.employee 存在如下记录：

```lang-json
{ "id": 1, "content": { "name": "Jack", "phone": "123", "address": "1000 Market Street, Philadelphia" } } 
{ "id": 2, "content": [ { "name": "Tom", "phone": "456", "address": "2000 Market Street, Philadelphia" } ] } 
{ "id": 3, "content": { "name": "Septem", "phone": "789", "address": { "addr1": "3000 Market Street, Philadelphia", "addr2": "4000 Market Street, Philadelphia" } } } 
{ "id": 4, "content": [ 80, 84, 90 ] } 
{ "id": 5, "content": [ 1, [ { a: 1 }, { b: 2 }, 90 ], 2, 3 ] }
```

* 匹配对象中的元素，例如：如果 content 对象中存在 name 为 Jack，phone 为 123 的元素，则返回该记录

  ```lang-javascript
  > db.sample.employee.find( { "content": { $elemMatch: { "name": "Jack", "phone": "123" } } } )
  {
      "_id": {
        "$oid": "5822868a2b4c38286d000007"
      },
      "id": 1,
      "content": {
        "name": "Jack",
        "phone": "123",
        "address": "1000 Market Street, Philadelphia"
      }
  }
  Return 1 row(s).
  ```

* 匹配数组中的嵌套元素，例如：如果 content 数组中存在 name 为 Tom，phone 为 456 的元素，则返回该记录

  ```lang-javascript
  > db.sample.employee.find( { "content": { $elemMatch: { "name": "Tom", "phone": "456" } } } )
  {
      "_id": {
        "$oid": "5822868a2b4c38286d000008"
      },
      "id": 2,
      "content": [
        {
          "name": "Tom",
          "phone": "456",
          "address": "2000 Market Street, Philadelphia"
        }
      ]
  }
  Return 1 row(s).
  ```

* 匹配数组中的非嵌套元素，例如：如果 content 数组存在值大于等于 80，小于等于 85 的元素，则返回该记录

  ```lang-javascript
  > db.sample.employee.find( { "content": { $elemMatch: { $gte: 80, $lte: 85 } } } )
  {
      "_id": {
        "$oid": "5822868a2b4c38286d000009"
      },
      "id": 4,
      "content": [
        80,
        84,
        90
      ]
  }
  Return 1 row(s).
  ```

* 匹配数组中的第 2 个元素，例如：如果 content 数组中的第二个元素中存在 a 为 1 的元素，则返回该记录

  ```lang-javascript
  > db.sample.employee.find( { "content.1": { $elemMatch: { a: 1 } } } )
  {
      "_id": {
        "$oid": "5fec23e6a13bedd56902eb5d"
      },
      "id": 5,
      "content": [
        1,
        [
          {
            "a": 1
          },
          {
            "b": 2
          },
          90
        ],
        2,
        3
      ]
  }
  Return 1 row(s).
  ```

* $elemMatch 可以和匹配符一起配合使用，例如：跟匹配符 $elemMatch 和 $regex 结合使用，content  对象（或数组）中的 address 对象（或数组）中，如果存在 addr1 满足正则表达式 ".*Philadelphia$" 的元素，则返回该记录

  ```lang-javascript
  > db.sample.employee.find( { "content" : { $elemMatch : { address : { $elemMatch: { addr1 : { $regex : ".*Philadelphia$" } } } } } } )
  {
      "_id": {
        "$oid": "5a0107641f9b983f4600000d"
      },
      "id": 3,
      "content": {
        "name": "Septem",
        "phone": "789",
        "address": {
          "addr1": "3000 Market Street, Philadelphia",
          "addr2": "4000 Market Street, Philadelphia"
        }
      }
  }
  Return 1 row(s).
  ```

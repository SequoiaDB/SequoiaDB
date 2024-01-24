
##语法##

```lang-json
{ <字段名>: { $et: <值> } }
```

##描述##

指定相等条件，$et 匹配符匹配记录中某个字段的值等于指定的值。

##示例###

在集合 sample.employee 插入如下记录：

```lang-javascript
> db.sample.employee.insert( { "id": 1, "item": { "name": "Tom",  "code": "123" } } )
> db.sample.employee.insert( { "id": 2, "item": { "name": "Jack", "code": "123" } } )
> db.sample.employee.insert( { "id": 3, "item": { "name": "Mike", "code": "456" } } )
> db.sample.employee.insert( { "id": 4, "item": { "name": "Lucy", "code": "789" }, "phone": [ "135", "177" ] } )
```

* 查询 id 字段值等于 1 的记录

  ```lang-javascript
  > db.sample.employee.find( { "id": { "$et": 1 } } )
  {
      "_id": {
        "$oid": "5822b0472b4c38286d000009"
      },
      "id": 1,
      "item": {
        "name": "Tom",
        "code": "123"
      }
  }
  ```

  查询相当于：

  ```lang-javascript
  > db.sample.employee.find( { "id": 1 } )
  ```

* 匹配嵌套字段 name 的值等于“Jack”的记录

  ```lang-javascript
  > db.sample.employee.find( { "item.name": { "$et": "Jack" } } )
  {
      "_id": {
        "$oid": "5822b04b2b4c38286d00000a"
      },
      "id": 2,
      "item": {
        "name": "Jack",
        "code": "123"
      }
  }
  Return 1 row(s).
  ```

  查询相当于：

  ```lang-javascript
  > db.sample.employee.find( { "item.name": "Jack" } )
  ```

* 匹配数组元素

  ```lang-javascript
  > db.sample.employee.find( { "phone": { "$et": "135" } } )
  {
          "_id": {
        "$oid": "5822b0562b4c38286d00000c"
      },
      "id": 4,
      "item": {
        "name": "Lucy",
        "code": "789"
      },
      "phone": [
        "135",
        "177"
      ]
  }
  Return 1 row(s).
  ```

  查询相当于：

  ```lang-javascript
  > db.sample.employee.find( { "phone": "135" } )
  ```


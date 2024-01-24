
##语法##

```lang-json
{ $and: [ { <表达式1> }，{ <表达式2> }, ... ] }
```

##描述##

$and 是一个逻辑“与”操作，用于选择满足所有表达式“[ <表达式1>, <表达式2>, ... ]”的记录，但是如果第一个表达式“<表达式1>”的计算结果为 false，SequoiaDB 将不会再执行后续的表达式。

##示例##

在集合 sample.employee 插入如下记录：

```lang-javascript
> db.sample.employee.insert( { "id": 100, "time": { "$date": "2012-10-01" }, "income": 10000, "expenses": 500 } )
> db.sample.employee.insert( { "id": 200, "time": { "$date": "2012-10-01" }, "income": 9000, "expenses": 1000 } )
```

* 查询 time 字段值大于或等于2010年1月1日，并且 income 字段值等于 10000 的记录

  ```lang-javascript
  > db.sample.employee.find( { "$and": [ { "time": { "$gte": { "$date": "2010-01-01" } } }, { "income": 10000 } ] } )
  {
      "_id": {
        "$oid": "5821afc52b4c38286d000003"
      },
      "id": 100,
      "time": {
        "$date": "2012-10-01"
      },
      "income": 10000,
      "expenses": 500
  }
  Return 1 row(s).
  ```

 >**Note:**
 >
 > 如果缺省逻辑符，默认是 $and 关系，查询结果跟使用 $and 的一致

  
* 查询字段 income 值大于 0，并且小于 10000 的记录

  当一个字段名有多个匹配条件时，可以用 $and 表达式，也可以合并表达式。

  ```lang-javascript
  > db.sample.employee.find( { "$and": [ { "income": { "$gt": 0 } }, { "income": { "$lt": 10000 } } ] } )
  {
      "_id": {
        "$oid": "5821afcd2b4c38286d000004"
      },
      "id": 200,
      "time": {
        "$date": "2013-05-01"
      },
      "income": 9000,
      "expenses": 1000
  }
  Return 1 row(s).
  ```

  或者

  ```lang-javascript
  > db.sample.employee.find( { "income": { "$gt": 0, "$lt": 10000 } } )
  {
      "_id": {
        "$oid": "5821afcd2b4c38286d000004"
      },
      "id": 200,
      "time": {
        "$date": "2013-05-01"
      },
      "income": 9000,
      "expenses": 1000
  }
  Return 1 row(s).
  ```


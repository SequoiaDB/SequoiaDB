
##语法##

```lang-json
{ <fieldName>: { $elemMatchOne: <cond> } }
```

##描述##

返回数组或者对象中满足条件的第一个元素

> **Note:**
>
> $elemMatchOne 匹配条件暂不支持全文检索语法

##示例##

集合 sample.employee 存在如下记录：

```lang-javascript
{ "id": 1, "class": 1, "students": [ { "name": "ZhangSan", "age": 18 }, { "name": "LiSi", "age": 19 }, { "name": "WangErmazi", "age": 18 } ] }
{ "id": 2, "class": 2, "students": { "name": "LinWu", "age": 18 } }
{ "id": 3, "class": 3, "students": [ { "name": "ZhangSan", "age": 18, course: [ { math: 1 }, { english: 0 } ] }, { "name": "LiSi", "age": 19, course: [ { math: 1 }, { english: 1 } ] }, { "name": "WangErmazi", "age": 18, course: [ { math: 0 }, { english: 0 } ] } ] }
{ "id": 4, "class": 4, "students": [ 80, 84, 90 ] }
```

* 选择数组中的嵌套元素，和对象中的元素，例如：如果 students 对象（或数组）中存在 age 为 18 的元素，则返回第一个被匹配到的对象（或数组）元素

  ```lang-javascript
  > db.sample.employee.find( {}, { "students": { "$elemMatchOne": { "age": 18 } } } )
  {
      "_id": {
        "$oid": "5fed304738adddbe0a69f455"
      },
      "id": 1,
      "class": 1,
      "students": [
        {
          "name": "ZhangSan",
          "age": 18
        }
      ]
    }
    {
      "_id": {
        "$oid": "5fed304c38adddbe0a69f456"
      },
      "id": 2,
      "class": 2,
      "students": {
        "age": 18
      }
    }
    {
      "_id": {
        "$oid": "5fed305138adddbe0a69f457"
      },
      "id": 3,
      "class": 3,
      "students": [
        {
          "name": "ZhangSan",
          "age": 18,
          "course": [
            {
              "math": 1
            },
            {
              "english": 0
            }
          ]
        }
      ]
  }
  {
      "_id": {
        "$oid": "5fed305838adddbe0a69f458"
      },
      "id": 4,
      "class": 4,
      "students": []
  }
  Return 4 row(s).
  ```

* 选择数组中的非嵌套元素，例如：如果 students 数组中存在值大于等于 80，小于等于 85 的元素，则返回第一个被匹配到的数组元素

  ```lang-javascript
  > db.sample.employee.find( {}, { "students": { "$elemMatchOne": { $gte: 80, $lte: 85 } } } )
  {
      "_id": {
        "$oid": "5fed304738adddbe0a69f455"
      },
      "id": 1,
      "class": 1,
      "students": []
  }
  {
      "_id": {
        "$oid": "5fed304c38adddbe0a69f456"
      },
      "id": 2,
      "class": 2
  }
  {
      "_id": {
        "$oid": "5fed305138adddbe0a69f457"
      },
      "id": 3,
      "class": 3,
      "students": []
  }
  {
      "_id": {
        "$oid": "5fed305838adddbe0a69f458"
      },
      "id": 4,
      "class": 4,
      "students": [
        80
      ]
  }
  Return 4 row(s).
  ```

* 选择符 $elemMatchOne 可以和匹配符一起组合使用，例如：跟匹配符 $elemMatch 结合使用，students 对象（或数组）中的 course 对象（或数组），如果存在 math 为 1 的元素，则返回第一个被匹配到的对象（或数组）元素

  ```lang-javascript
  > db.sample.employee.find( { class: 3 }, { students: { $elemMatchOne: { course: { $elemMatch: { math: 1 } } } } } )
  {
      "_id": {
        "$oid": "5fed305138adddbe0a69f457"
      },
      "id": 3,
      "class": 3,
      "students": [
        {
          "name": "ZhangSan",
          "age": 18,
          "course": [
            {
              "math": 1
            },
            {
              "english": 0
            }
          ]
        }
      ]
  }
  Return 1 row(s).
  ```

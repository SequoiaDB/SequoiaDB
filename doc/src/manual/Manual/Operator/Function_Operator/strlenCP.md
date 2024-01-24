##语法##

```lang-json
{<字段名>: {$strlenCP: 1}}
```

##说明##

$strlenCP 用于获取指定字段的字符数，即[代码点][codePoint]的数量。当字段类型为字符串时，将按 UTF-8 编码规则返回对应字符数；当字段类型为非字符串时，将返回 null。

##示例##

在集合 sample.employee 插入如下记录：

```lang-javascript
> db.sample.employee.insert({_id: 1, "a": "12345"})
> db.sample.employee.insert({_id: 2, "a": "一二三四五"})
> db.sample.employee.insert({_id: 3, "a": "一二三45"})
> db.sample.employee.insert({_id: 4, "a": ["12345", "一二三四五", "一二三45"]})
```

* 作为选择符使用，返回字段 a 的字符数

    ```lang-javascript
    > db.sample.employee.find({}, {"a": {"$strlenCP": 1}})
    {
        "_id": 1,
        "a": 5
    }
    {
        "_id": 2,
        "a": 5
    }
    {
        "_id": 3,
        "a": 5
    }
    {
        "_id": 4,
        "a": [
          5,
          5,
          5
        ]
    }
    Return 4 row(s).
    ```

* 配合匹配符使用，返回集合中字段 a 的字符数为 5 的记录
  
    ```lang-javascript
    > db.sample.employee.find({"a": {"$strlenCP": 1, "$et": 5}})
    {
        "_id": 1,
        "a": "12345"
    }
    {
        "_id": 2,
        "a": "一二三四五"
    }
    {
        "_id": 3,
        "a": "一二三45"
    }
    {
        "_id": 4,
        "a": [
          "12345",
          "一二三四五",
          "一二三45"
        ]
    }
    Return 2 row(s).
    ```
    
    > **Note:**  
    >
    > {$strlenCP: 1} 中 1 没有特殊含义，仅作为占位符出现。

[^_^]:
    本文使用的所有引用及链接
[codePoint]:http://www.unicode.org/glossary/#code_point
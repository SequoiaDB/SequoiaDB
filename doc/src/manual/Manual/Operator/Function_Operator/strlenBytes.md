
##语法##

```lang-json
{<字段名>: {$strlenBytes: 1}}
```

##说明##

$strlenBytes 用于获取指定字段的字节数。当字段类型为字符串时，将按 UTF-8 编码规则返回对应字节数；当字段类型为非字符串时，将返回 null。

##示例##

在集合 sample.employee 插入如下记录：

```lang-javascript
> db.sample.employee.insert({_id: 1, "a": "12345"})
> db.sample.employee.insert({_id: 2, "a": "一二三四五"})
> db.sample.employee.insert({_id: 3, "a": "一二三45"})
> db.sample.employee.insert({_id: 4, "a": ["12345", "一二三四五", "一二三45"]})
```

* 作为选择符使用，返回字段 a 的字节数

    ```lang-javascript
    > db.sample.employee.find({}, {"a": {"$strlenBytes": 1}})
    {
        "_id": 1,
        "a": 5
    }
    {
        "_id": 2,
        "a": 15
    }
    {
        "_id": 3,
        "a": 11
    }
    {
        "_id": 4,
        "a": [
          5,
          15,
          11
        ]
    }
    Return 4 row(s).
    ```

* 配合匹配符使用，返回集合中字段 a 的字节数为 15 的记录
  
    ```lang-javascript
    > db.sample.employee.find({"a": {"$strlenBytes": 1, "$et": 15}})
    {
        "_id": 2,
        "a": "一二三四五"
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
    > {$strlenBytes: 1} 中 1 没有特殊含义，仅作为占位符出现。


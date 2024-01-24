
##语法##

返回从“<起始下标>”开始到数组末尾的元素：

```lang-json
{ <字段名>: { $returnMatch: <起始下标> } }
```

返回从“<起始下标>”开始，长度为指定“<长度>”的元素：

```lang-json
{ <字段名>: { $returnMatch: [ <起始下标>, <长度> ] } }
```

##描述##

$returnMatch 选取匹配成功的数组元素，可以通过参数选取指定的元素，必须结合对数组做展开匹配的匹配符使用($in, $all等，不支持 $elemMatch)。

##示例##

集合 sample.employee 存在如下记录：

```lang-javascript
> db.sample.employee.find()
{
  "a": [
    1,
    2,
    4,
    7,
    9
  ]
}
```

* 查询集合 sample.employee 中 a 字段数组元素的值为 1 或 4 或 7 的记录

  ```lang-javascript
  > db.sample.employee.find({a:{$in:[1,4,7]}})
  {
      "a": [
        1,
        2,
        4,
        7,
        9
      ]
  }
  ```

  > **Note:**
  >
  > 不带 $returnMatch，默认返回原始记录，包括没有匹配到的元素 2 和 9。

* 查询集合 sample.employee 中 a 字段数组元素的值为 1 或 4 或 7 的记录，并选取命中的所有元素

  ```lang-javascript
  > db.sample.employee.find({a:{$returnMatch:0, $in:[1,4,7]}})
  {
      "a": [
        1,
        4,
        7
      ]
  }
  ```

  > **Note:**
  >
  > { $returnMatch:0 }表示选取下标为 0 开始的所有命中的元素，不包括没有命中的元素 2 和 9。

* 查询集合 sample.employee 中 a 字段数组元素的值为 1 或 4 或 7 的记录，并选取第 2、3 个命中的元素

  ```lang-javascript
  > db.sample.employee.find({a:{$returnMatch:[1,2], $in:[1,4,7]}})
  {
      "a": [
        4,
        7
      ]
  }
  ```

  > **Note:**
  >
  > { $returnMatch:[1,2] }表示选取下标为 1 开始，长度为 2 的所有命中的元素。



SYSPROCEDURES.STOREPROCEDURES 集合中包含了所有的存储过程函数，每一个函数保存为一个文档。

每个文档包含以下字段：

| 字段名          | 类型   | 描述                                              |
|-----------------|--------|---------------------------------------------------|
| name            | string | 函数名                                            |
| func            | string | 函数体                                            |
| funcType        | number | 函数类型 <br> 0：代表 JavaScript 函数 <br> 其他类型暂无|

##示例##

一个简单的存储过程函数如下：

```lang-json
{
  "_id" : { "$oid" : "5257b115925c31dd16ec4e4a" },
  "name" : "fun",
  "func" : "function fun(num) {
      if (num == 1) {
          return 1;
      } else {
          return fun(num - 1) * num;
      }
  }",
  "funcType" : 0
}
```


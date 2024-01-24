## 语法

```lang-json
{ $rename: { <name>:<newname>[,<name>:<newname>, ... ]} }
```

## 描述

$rename 操作符是将字段名 name 重命名为字段名 newname。

## 示例

* 集合 sample.employee 存在如下记录：

 ```lang-json
 { "a": 5,"b": 10 }
 ```

 将字段名 a 重命名为字段名 c，将字段名 b 重命名为字段名 d：

 ```lang-javascript
 > db.sample.employee.update({$rename:{'a':'c','b':'d'}})
 ```

 此操作后，记录更新为：

 ```lang-json
 { "c": 5,"d": 10 }
 ```
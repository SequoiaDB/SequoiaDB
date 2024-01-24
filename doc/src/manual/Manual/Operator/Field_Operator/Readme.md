字段操作符可以实现对字段值进行匹配查询、修改等功能，SequoiaDB 巨杉数据库支持以下字段操作符：

| 参数名 | 描述 | 示例 |
| ------ | ---- | ---- |
| [$field][field] | 取出指定字段的值，应用于其它操作 | 	({$inc: {t1: {Value: {$field: "t2"}}}}) |


[^_^]:
     本文使用的所有引用及连接
[field]:manual/Manual/Operator/Field_Operator/field.md

选择符可以实现对查询的结果字段进行拆分、筛选等功能。在选择符中使用[函数操作][overview]还能实现对结果的重写和转换等操作。

支持的选择符如下：

| 选择符                                                          | 描述                     | 示例                                              |
| --------------------------------------------------------------- | ------------------------ | ------------------------------------------------- |
| [$include][include]     | 选择或移除某个字段       | db.sample.employee.find({}, {"a": {"$include": 1}})     |
| [$default][default]     | 当字段不存在时返回默认值 | db.sample.employee.find({}, {"a": {"$default": "myvalue"}}) |
| [$elemMatch][elemMatch] | 返回数组或者对象中满足条件的元素集合 | db.sample.employee.find({}, {"a": {"$elemMatch": {"a": 1}}}) |
| [$elemMatchOne][elemMatchOne] | 返回数组或者对象中满足条件的第一个元素 | db.sample.employee.find({}, {"a": {"$elemMatchOne": {"a": 1}}})     |




[^_^]:
    本文使用的所有引用及链接
[overview]:manual/Manual/Operator/Function_Operator/Readme.md
[include]:manual/Manual/Operator/Selector_Operator/include.md
[default]:manual/Manual/Operator/Selector_Operator/default.md
[elemMatch]:manual/Manual/Operator/Selector_Operator/elemMatch.md
[elemMatchOne]:manual/Manual/Operator/Selector_Operator/elemMatchOne.md
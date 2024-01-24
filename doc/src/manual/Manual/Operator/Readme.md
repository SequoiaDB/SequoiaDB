操作符表示表达式与其他元素之间特定的算数或逻辑关系，为数据操作指定更明确的条件。SequoiaDB 巨杉数据库支持如下操作符：

- [匹配符][match]：指定匹配条件，使查询仅返回符合条件的记录
- [选择符][selector]：实现对查询的结果字段进行拆分、筛选等功能
- [函数操作][function]：配合匹配符和选择符使用，以实现更复杂的功能
- [更新符][update]：实现对字段的添加、修改和删除操作
- [聚集符][aggregate]：配合 aggregate() 使用，计算集合中数据的聚合值
- [字段操作符][field]：实现对字段值进行匹配查询、修改等功能


[^_^]:
     本文使用的所有引用及链接
[match]:manual/Manual/Operator/Match_Operator/Readme.md
[selector]:manual/Manual/Operator/Selector_Operator/Readme.md
[function]:manual/Manual/Operator/Function_Operator/Readme.md
[update]:manual/Manual/Operator/Update_Operator/Readme.md
[aggregate]:manual/Manual/Operator/Aggregate_Operator/Readme.md
[field]:manual/Manual/Operator/Field_Operator/Readme.md
##名称##

attachCL - 挂载子分区集合

##语法##

**db.collectionspace.collection.attachCL(\<subCLFullName\>, \<options\>)**

##类别##

SdbCollection

##描述##

该函数用于在主分区集合下挂载子分区集合。

##参数##

| 参数名 | 类型 | 描述 | 是否必填 |
| ------ | ------ | ------ | ------ |
| subCLFullName | string | 子分区集合名（包含集合空间名） | 是 |
| options | object |  分区范围，包含两个字段“LowBound”（区间左值）以及“UpBound”（区间右值），例如：`{LowBound: {a: 0}, UpBound: {a: 100}}` 表示取字段“a”的范围区间：[0, 100) | 是 |

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`attachCL()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| ------ | -------- | -------------- | -------- |
| -236   |SDB_INVALID_MAIN_CL| 无效的分区集合 | 检查主分区集合信息是否正确，主分区集合需要设置属性 IsMainCL为true |
| -23    |SDB_DMS_NOTEXIST| 集合不存在     | 检查子分区集合是否存在，如果不存在请创建对应的子分区集合 |
| -237   |SDB_BOUND_CONFLICT| 新增区间与现有区间冲突 |查看现有区间，修改新增区间范围|

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.0 及以上版本

##示例##

在主分区集合的指定区间下挂载子分区集合

```lang-javascript
> db.sample.employee.attachCL("sample2.January", {LowBound: {date: "20130101"}, UpBound: {date: "20130131"}})
```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md

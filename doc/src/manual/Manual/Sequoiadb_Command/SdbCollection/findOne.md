##名称##

findOne - 查询符合条件的一条记录

##语法##

**db.collectionspace.collection.findOne\(\[cond\], \[sel\]\)**

**db.collectionspace.collection.findOne([cond], [sel]).hint([hint])**

**db.collectionspace.collection.findOne([cond], [sel]).skip([skipNum]).limit([retNum]).sort([sort])**

**db.collectionspace.collection.findOne([SdbQueryOption])**

##类别##

SdbCollection

##描述##

该函数用于返回符合查询条件的一条记录，与 [find()][find] 方法用法相同。

##参数##

参数说明可参考 [find()][find] 方法。

##返回值##
  
函数执行成功时，将返回一个 SdbQuery 的对象。通过该对象获取查询到的结果集。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`findOne()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| ------ | -------- | -------------- | -------- |
| -2 | SDB_OOM | 无可用内存| 检查物理内存及虚拟内存的设置及使用情况|
| -6 | SDB_INVALIDARG | 参数错误 | 查看参数是否填写正确|
| -34 | SDB_DMS_CS_NOTEXIST | 集合空间不存在| 检查集合空间是否存在|
| -23 | SDB_DMS_NOTEXIST| 集合不存在 | 检查集合是否存在|

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.4 及以上版本

##示例##

- 查询所有记录，不指定 cond 和 sel 字段

    ```lang-javascript
    > db.sample.employee.findOne()
    ```

- 查询匹配条件的记录，即设置 cond 参数的内容，如下操作返回集合 employee 中符合条件 age 字段值大于 25 且 name 字段值为"Tom"的记录

    ```lang-javascript
    > db.sample.employee.findOne({age: {$gt: 25}, name: "Tom"})
     ```

- 指定返回的字段名，即设置 sel 参数的内容，如有记录 {age: 25, type: "system"} 和 {age: 20, name: "Tom", type: "normal"}，如下操作返回记录的 age 字段和 name 字段

    ```lang-javascript
    > db.sample.employee.findOne(null, {age: "", name: "" })
        {
            "age": 25,
            "name": ""
        }
    ```

- 使用索引 ageIndex 遍历集合 employee 下存在 age 字段的记录，并返回

    ```lang-javascript
    > db.sample.test.findOne({age: {$exists: 1}}).hint({"": "ageIndex"})
    {
            "_id": {
            "$oid": "5812feb6c842af52b6000007"
            },
            "age": 10
    }
    ```

- 返回集合 employee 中 age 字段值大于 20 的记录（如使用 [$gt][gt] 查询），设置只返回记录的 name 和 age 字段，并按 age 字段值的升序排序

    ```lang-javascript
    > db.sample.employee.findOne({age: {$gt: 20}}, {age: "", name: ""}).sort({age: 1})
    ```

    通过 findOne() 方法，我们能任意选择我们想要返回的字段名，在上例中我们选择了返回记录的 age 和 name 字段，此时用 sort() 方法时，只能对记录的 age 或 name 字段排序。而如果我们选择返回记录的所有字段，即不设置 findOne 方法的 sel 参数内容时，那么 sort() 能对任意字段排序。

- 指定一个无效的排序字段

    ```lang-javascript
    > db.sample.employee.findOne({age: {$gt: 20}}, {age: "", name: ""}).sort({"sex": 1})
    ```

	因为“sex”字段并不存在 findOne() 方法的 sel 选项 {age: "", name: ""} 中，所以 sort() 指定的排序字段 {"sex": 1} 将被忽略。

[^_^]:
    本文使用的所有引用及链接
[overview]:manual/Manual/Operator/Match_Operator/Readme.md
[text_index]:manual/Distributed_Engine/Architecture/Data_Model/text_index.md
[QueryOption]:manual/Manual/Sequoiadb_Command/AuxiliaryObjects/SdbQueryOption.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[faq]:manual/FAQ/faq_sdb.md
[find]:manual/Manual/Sequoiadb_Command/SdbCollection/find.md

[gt]:manual/Manual/Operator/Match_Operator/gt.md
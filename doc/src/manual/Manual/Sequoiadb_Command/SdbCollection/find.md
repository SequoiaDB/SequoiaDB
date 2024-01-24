##名称##

find - 查询记录

##语法##

**db.collectionspace.collection.find([cond], [sel])**

**db.collectionspace.collection.find([cond], [sel]).hint([hint])**

**db.collectionspace.collection.find([cond], [sel]).hint([hint]).flags(\<flags\>)**

**db.collectionspace.collection.find([cond], [sel]).skip([skipNum]).limit([retNum]).sort([sort])**

**db.collectionspace.collection.find([cond], [sel])[.hint([hint])][.skip([skipNum])][.limit([retNum])][.sort([sort])]**

**db.collectionspace.collection.find([SdbQueryOption])**

##类别##

SdbCollection

##描述##

该函数用于返回符合查询条件的记录。

##参数##

- cond ( *object，选填* )

	记录匹配条件。为空时，查询所有记录；不为空时，查询符合条件记录。如：{"age":{"$gt":30}}。匹配条件可使用[匹配符][overview]或[全文检索语法][text_index]。

- sel ( *object，选填* )

	查询返回记录的字段名。为空时，返回记录的所有字段；如果指定的字段名记录中不存在，则按用户设定的内容原样返回。如：{"name":"","age":"","addr":""}。

- hint ( *object，选填* )

	指定查询使用索引的情况。
	* 不指定 `hint` ：查询是否使用索引及使用哪个索引将由数据库决定；
	* `hint` 为{"":null}：查询走表扫描；
	* `hint` 为单个索引：如：{"":"myIdx"}，表示查询将使用当前集合中名字为 "myIdx" 的索引进行；
	* `hint` 为多个索引：如：{"1":"idx1","2":"idx2","3":"idx3"}，
                        表示查询将使用上述三个索引之一进行。
                        具体使用哪一个，由数据库评估决定。

- flags ( *object，选填* )

    指定标志位遍历结果集，具体用法可参考 [flags()][flags]。

- skipNum ( *number，选填* )

	自定义从结果集哪条记录开始返回。默认值为0，表示从第一条记录开始返回。

- retNum ( *number，选填* )

	自定义返回结果集的记录条数。默认值为-1，表示返回从 `skipNum` 位置开始到结果集结束位置的所有记录。

- sort ( *object，选填* )

	指定结果集按指定字段名排序的情况。字段名的值为 1 或者 -1，如：{"name":1,"age":-1}。
	* 不指定 `sort` ：表示不对结果集做排序；
	* 字段名的值为 1：表示按该字段名升序排序；
	* 字段名的值为 -1：表示按该字段名降序排序。

- SdbQueryOption ( *object，选填* )

	使用一个对象来指定记录查询参数。使用方法可参考 [SdbQueryOption][QueryOption]。

> **Note：**
>
> * `sel` 参数为 Object 类型，其字段内容为空字符串即可，数据库只关心其字段名。
> * `hint` 参数为 Object 类型，其字段名可以为任意不重复的字符串，数据库只关心起字段内容。 

##返回值##
  
函数执行成功时，将返回一个 SdbQuery 类型的对象。通过该对象获取查询到的结果集。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`find()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| ------ | -------- | -------------- | -------- |
| -2 | SDB_OOM | 无可用内存| 检查物理内存及虚拟内存的设置及使用情况|
| -6 | SDB_INVALIDARG | 参数错误 | 查看参数是否填写正确|
| -34 | SDB_DMS_CS_NOTEXIST | 集合空间不存在| 检查集合空间是否存在|
| -23 | SDB_DMS_NOTEXIST| 集合不存在 | 检查集合是否存在|

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v1.0 及以上版本

##示例##

- 查询所有记录，不指定 cond 和 sel 字段。

    ```lang-javascript
    > db.sample.employee.find()
    ```

- 查询匹配条件的记录，即设置 cond 参数的内容。如下操作返回集合 employee 中符合
   条件 age 字段值大于 25 且 name 字段值为 "Tom" 的记录。

    ```lang-javascript
    db.sample.employee.find( { age: { $gt: 25 }, name: "Tom" } )
     ```

- 指定返回的字段名，即设置 sel 参数的内容。如有记录{ age: 25, type: "system" }
   和{ age: 20, name: "Tom", type: "normal" }，如下操作返回记录的age字段和name字段。

    ```lang-javascript
    db.sample.employee.find( null, { age: "", name: "" } )
        {
            "age": 25,
            "name": ""
        }
        {
            "age": 20,
            "name": "Tom"
        }
    ```

- 使用索引 ageIndex 遍历集合 employee 下存在 age 字段的记录，并返回。

    ```lang-javascript
    > db.sample.test.find( {age: {$exists:1} } ).hint( { "": "ageIndex" } )
    {
            "_id": {
            "$oid": "5812feb6c842af52b6000007"
            },
            "age": 10
    }
     {
            "_id": {
            "$oid": "5812feb6c842af52b6000008"
            },
            "age": 20
    }
    ```

- 选择集合 employee 下 age 字段值大于 10 的记录（如使用 [$gt][gt] 查询），从第 5 条记录开始返回，即跳过前面的四条记录。

    ```lang-javascript
    > db.sample.employee.find( { age: { $gt: 10 } } ).skip(3).limit(5)
    ```

    如果结果集的记录数不大于 3，那么无记录返回；
    如果结果集的记录数大于 3，则从第 4 条开始, 最多返回 5 条记录。

- 返回集合 employee 中 age 字段值大于 20 的记录（如使用 [$gt][gt] 查询），设置只返回记录的 name 和 age 字段，并按 age 字段值的升序排序。

    ```lang-javascript
    db.sample.employee.find( { age: { $gt: 20 } }, { age: "", name: "" } ).sort( { age: 1 } )
    ```

    通过 find() 方法，我们能任意选择我们想要返回的字段名，在上例中我们选择了返回记录的 age 和 name 字段，此时用 sort() 方法时， 只能对记录的 age 或 name 字段排序。而如果我们选择返回记录的所有字段，即不设置 find 方法的 sel 参数内容时，那么 sort() 能对任意字段排序。

- 指定一个无效的排序字段。

    ```lang-javascript
    db.sample.employee.find( { age: { $gt: 20 } }, { age: "", name: "" } ).sort( { "sex": 1 } )
    ```

    因为“sex”字段并不存在 find() 方法的 sel 选项 {age:"",name:""} 中，
    所以 sort() 指定的排序字段 {"sex":1} 将被忽略。

- 使用全文检索语法查询集合 "employee" 中的 "about" 字段包含 "rock climbing" 的记录。

    ```lang-javascript
    > db.sample.employee.find({"":{"$Text":{"query":{"match":{"about" : "rock climbing"+}}}}})
    ```


[^_^]:
    本文使用的所有引用及链接
[overview]:manual/Manual/Operator/Match_Operator/Readme.md
[text_index]:manual/Distributed_Engine/Architecture/Data_Model/text_index.md
[QueryOption]:manual/Manual/Sequoiadb_Command/AuxiliaryObjects/SdbQueryOption.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[faq]:manual/FAQ/faq_sdb.md
[gt]:manual/Manual/Operator/Match_Operator/gt.md
[flags]:manual/Manual/Sequoiadb_Command/SdbQuery/flags.md
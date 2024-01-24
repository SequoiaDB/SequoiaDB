##名称##

insert - 将记录插入当前集合

##语法##

**db.collectionspace.collection.insert(\<doc|docs\>, [flag])**

**db.collectionspace.collection.insert(\<doc|docs\>, [options])**

##类别##

SdbCollection

##描述##

该函数用于将单条或多条记录插入当前集合。

##参数##

- doc|docs（ *object/array，必填* ）

    单条或者多条记录

- flag（ *number，选填* ）

    标志位，用于控制插入操作的行为及结果。如果不指定该参数，插入操作默认不返回字段 _id 的内容，且发生索引键冲突时将报错。

    取值如下：
	
    - SDB_INSERT_RETURN_ID：插入成功后返回记录中字段 _id 的内容。
    - SDB_INSERT_CONTONDUP：发生索引键冲突时，忽略该条新记录并继续插入其他记录。
    - SDB_INSERT_REPLACEONDUP：发生索引键冲突时，新记录将覆盖原有记录，并继续插入其他记录。
    - SDB_INSERT_CONTONDUP_ID：发生 $id 索引键冲突时，忽略该条新记录并继续插入其他记录。
    - SDB_INSERT_REPLACEONDUP_ID：发生 $id 索引键冲突时，新记录将覆盖原有记录，并继续插入其他记录。

    >**Note：**
    >
    > - SDB_INSERT_RETURN_ID 支持与其他标志位同时指定，多个取值间用“|”分隔。
    > - 对于 SDB_INSERT_CONTONDUP、SDB_INSERT_REPLACEONDUP、SDB_INSERT_CONTONDUP_ID 和 SDB_INSERT_REPLACEONDUP_ID，不支持同时指定多项。

- options（ *object，选填* ）

    通过参数 options 可以控制插入操作的行为及结果：
 
    - ReturnOID（ *boolean* ）：与参数 flag 中的 SDB_INSERT_RETURN_ID 行为一致

        格式：`ReturnOID: true`

    - ContOnDup（ *boolean* ）：与参数 flag 中的 SDB_INSERT_CONTONDUP 行为一致

        格式：`ContOnDup: true`

    - ReplaceOnDup（ *boolean* ）：与参数 flag 中的 SDB_INSERT_REPLACEONDUP 行为一致

        格式：`ReplaceOnDup: true`

    - ContOnDupID（ *boolean* ）：与参数 flag 中的 SDB_INSERT_CONTONDUP_ID 行为一致

        格式：`ContOnDupID: true`

    - ReplaceOnDupID（ *boolean* ）：与参数 flag 中的 SDB_INSERT_REPLACEONDUP_ID 行为一致

        格式：`ReplaceOnDupID: true`

    >**Note:**
    >
    > - 如果不指定参数 options，插入操作默认不返回字段 _id 的内容，且发生索引键冲突时将报错。
    > - 对于参数 ContOnDup、ReplaceOnDup、ContOnDupID 和 ReplaceOnDupID，不支持同时指定多项为 true。

##返回值##

函数执行成功时，将返回一个 BSONObj 类型的对象，通过该对象获取成功插入的记录数信息，字段说明如下：

| 字段名 | 类型 | 描述 |
|--------|------|------|
| InsertedNum | int64 | 成功插入的记录数（不包含被覆盖的记录） |
| DuplicatedNum | int64 | 因索引键冲突被忽略或覆盖的记录数 |
| LastGenerateID | int64 | 自增字段的值（仅在集合包含[自增字段][auto-increment]时显示），返回情况如下：<br> - 当插入单条记录时，返回该记录所对应的自增字段值<br>- 当插入多条记录时，仅返回第一条记录对应的自增字段值<br>- 当存在多个自增字段时，插入单条记录，仅返回所有自增字段中的最大值 <br> - 当存在多个自增字段时，插入多条记录，仅返回第一条记录所对应的最大自增字段值 |
| _id | oid | 返回插入的记录中字段 _id 所包含的内容（仅参数 flag 取值为 SDB_INSERT_RETURN_ID 或参数 ReturnOID 为 true 时显示 ）|

函数执行失败时，将抛异常并输出错误信息。

##错误##

`insert()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| ------ | -------- | -------------- | -------- |
| -6 | SDB_INVALIDARG | 参数错误 | 查看参数是否填写正确 |
| -23 | SDB_DMS_NOTEXIST| 集合不存在 | 检查集合是否存在 |
| -34 | SDB_DMS_CS_NOTEXIST | 集合空间不存在 | 检查集合空间是否存在 |
| -38 | SDB_IXM_DUP_KEY | 索引键已存在 | 检查插入记录的索引键是否存在 |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.4 及以上版本

##示例##

- 在集合 sample.employee 中插入一条记录

    ```lang-javascript
    > db.sample.employee.insert({name: "Tom", age: 20})
    ```

- 在集合 sample.employee 中插入多条记录

 	```lang-javascript
 	> db.sample.employee.insert([{_id: 20, name: "Mike", age: 15}, {name: "John", age: 25, phone: 123}])
 	```

- 在集合 sample.employee 中插入拥有重复 _id 键的多条记录，并指定参数 flag 为 SDB_INSERT_CONTONDUP

    ```lang-javascript
    
    > db.sample.employee.insert([{_id: 1, a: 1}, {_id: 1, b: 2}, {_id: 3, c: 3}], SDB_INSERT_CONTONDUP)
    > db.sample.employee.find()
    {
      "_id": 1,
      "a": 1,
    }
    {
      "_id": 3,
      "c": 3
    }
    ```

- 在集合 sample.employee 中插入多条记录，并指定参数 ReturnOID 为 true

    ```lang-javascript
    > db.sample.employee.insert([{a: 1}, {b: 1}], {ReturnOID: true})
    {
        "_id": [
            {
                "$oid": "5bececdf6404b9295a63cacb"
            },
            {
                "$oid": "5bececdf6404b9295a63cacc"
            }
        ]
        "InsertedNum": 2,
        "DuplicatedNum": 0
    }
    ```

- 在集合 sample.employee 中创建自增字段，并插入一条记录

    ```lang-javascript
    > db.sample.employee.createAutoIncrement({Field: "ID"})
    > db.sample.employee.insert({a: 1})
    {
        "InsertedNum": 1,
        "DuplicatedNum": 0,
        "LastGenerateID": 1
    }
    ```
 
[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[auto-increment]:manual/Distributed_Engine/Architecture/Data_Model/sequence.md
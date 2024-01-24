##名称##

createIndex - 创建索引

##语法##

**db.collectionspace.collection.createIndex(\<name\>, \<indexDef\>, [isUnique], [enforced], [sortBufferSize])**

**db.collectionspace.collection.createIndex(\<name\>, \<indexDef\>, [indexAttr], [option])**

##类别##

SdbCollection

##描述##

该函数用于为集合创建[索引][index]，以提高查询速度。创建前用户需了解索引的相关[限制][limitation]。

##参数##

- name（ *string，必填* ）

    索引名，同一个集合中的索引名必须唯一

- indexDef（ *object，必填* ）

    索引键，格式为 `{<索引字段>:<类型值>, ...}`，其中可选类型值如下：

    - 1：按字段值升序排序
    - -1：按字段值降序排序
    - "text"：创建[全文索引][text_index]

- isUnique（ *boolean，选填* ）

    是否为唯一索引，默认值为 false，表示不为唯一索引

- enforced（ *boolean，选填* ）

    是否强制唯一，默认值为 false

    - 取值为 true 时，不能重复插入索引字段值为 null 的记录。
    - 仅在参数 isUnique 为 true 时生效。

- sortBufferSize（ *number，选填* ）

    排序缓存的大小，默认值为 64，单位为 MB

    - 取值为 0 时，表示不使用排序缓存。
    - 当集合的记录数大于 1000 万条时，适当增大排序缓存大小可提高创建索引的速度。

> **Note:**
>
> 对于全文索引，参数 isUnique、enforced 和 sortBufferSize 不生效。

- indexAttr（ *object，选填* ）

    通过参数 indexAttr 可以设置索引属性：

    - Unique（ *boolean* ）：是否为唯一索引，默认值为 false，表示不为唯一索引

        格式：`Unique: true`

    - Enforced（ *boolean* ）：是否强制唯一，默认值为 false

        - 取值为 true 时，不能重复插入索引字段值为 null 的记录。
        - 仅在参数 Unique 为 true 时生效
                            
        格式：`Enforced: true`

    - NotNull（ *boolean* ）：插入记录时是否允许索引字段不存在或取值为 null，默认值为 false

        取值如下：

        - true：索引字段必须存在且取值不能为 null
        - false：允许索引字段不存在或取值为 null

        格式：`NotNull: true`

    - NotArray（ *boolean* ）：插入记录时是否允许索引字段的取值为数组，默认值为 false

        取值如下：

        - true：索引字段的值不允许为数组
        - false：索引字段的字段值允许为数组

        格式：`NotArray: true`

    - Standalone（ *boolean* ）：是否为独立索引，默认值为 false，表示不为独立索引

        该参数指定为 true 时，必须指定参数 NodeName、NodeID 或 InstanceID。

        格式：`Standalone: true`

- option（ *object，选填* ）
 
    通过参数 option 可以设置控制参数：

    - SortBufferSize（ *number* ）：排序缓存的大小，默认值为 64，单位为 MB

        - 取值为 0 时，表示不使用排序缓存。
        - 当集合的记录数大于 1000 万条时，适当增大排序缓存大小可提高创建索引的速度。

        格式：`SortBufferSize: 80`

    - NodeName（ *string/array* ）：数据节点名

        格式：`NodeName: "sdbserver:11820"`
     
    - NodeID（ *number/array* ）：数据节点 ID

        格式：`NodeID: 1001`

    - InstanceID（ *number/array* ）：数据节点的实例 ID

        格式：`InstanceID: 100`

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.0 及以上版本

##示例##

- 创建名为“ageIndex”的唯一索引，并指定记录按索引字段 age 的值升序排序

    ```lang-javascript
    > db.sample.employee.createIndex("ageIndex", {age: 1}, true)
    ```

- 创建名为“addr_tags”的全文索引

    ```lang-javascript
    > db.sample.employee.createIndex("addr_tags", {address: "text", tags: "text"})
    ```

- 创建名为“ab”的唯一索引，并指定参数 NotNull 为 true

    ```lang-javascript
    > db.sample.employee.createIndex("ab", {a: 1, b: 1}, {Unique: true, NotNull: true})
    ```

    当字段 b 不存在或取值为 null 时，将返回报错信息

    ```lang-javascript
    > db.sample.employee.insert({a: 1})
    sdb.js:625 uncaught exception: -339
    Any field of index key should exist and cannot be null
    
    > db.sample.employee.insert({a: 1, b: null})
    sdb.js:625 uncaught exception: -339
    Any field of index key should exist and cannot be null
    ```

- 创建名为“ab”的索引，并指定参数 NotArray 为 true

    ```lang-javascript
    > db.sample.employee.createIndex("ab", {a: 1, b: 1}, {NotArray: true})
    ```

    当字段 a 为数组时，将返回报错信息

    ```lang-javascript
    > db.sample.employee.insert({a: [1], b: 10})
    sdb.js:645 uncaught exception: -364
    Any field of index key cannot be array
    ```

- 在数据节点 `sdbserver:11850` 上创建名为“a”的独立索引

    ```lang-javascript
    > db.sample.employee.createIndex("a", {a: 1}, {Standalone: true}, {NodeName: "sdbserver:11850"})
    ```

[^_^]:
     本文使用的所有引用和链接
[index]:manual/Distributed_Engine/Architecture/Data_Model/index.md
[limitation]:manual/Manual/sequoiadb_limitation.md#索引
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[text_index]:manual/Distributed_Engine/Architecture/Data_Model/text_index.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
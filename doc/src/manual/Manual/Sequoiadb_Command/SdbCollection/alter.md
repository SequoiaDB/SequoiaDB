##名称##

alter - 修改集合的属性

##语法##

**db.collectionspace.collection.alter(\<options\>)**

##类别##

SdbCollection

##描述##

该函数用于修改集合的属性。详情可参考 [setAttributes\(\)][setAttributes]。

##参数##

options ( *object，必填* )

通过 options 参数可以修改集合属性：

- ReplSize ( *number* )：写操作需同步的副本数，其可选取值如下：

    - -1：表示写请求需同步到该复制组若干活跃的节点之后，数据库写操作才返回应答给客户端。
    - 0：表示写请求需同步到该复制组的所有节点之后，数据库写操作才返回应答给客户端。
    - 1 ~ 7：表示写请求需同步到该复制组指定数量个节点之后，数据库写操作才返回应答给客户端。

    格式：`ReplSize: <number>`

- ConsistencyStrategy（ *number* ）：[同步一致性][consistency_strategy]策略

    该参数用于指定数据同步优先选择的节点，默认值为 3。

    取值如下：

    - 1：节点优先策略
    - 2：位置多数派优先策略
    - 3：主位置多数派优先策略

    格式：`ConsistencyStrategy: 3`

- ShardingKey ( *object* )：分区键，取值为 1 或 -1，表示正向或逆向排序

    - 已有的 ShardingKey 会被修改成新的 ShardingKey。
    - 集合只能存在于一个数据组中，或者集合为没有挂载子表的主表。

    格式：`ShardingKey: {<字段1>: <1|-1>, [<字段2>: <1|-1>, ...]}`

- ShardingType ( *string* )：分区方式，默认为 hash 分区，其可选取值如下：

    - "hash"：hash 分区
    - "range"：范围分区

    集合只能存在于一个数据组中。

    格式：`ShardingType: "hash" | "range"`

- Partition ( *number* )：分区数，仅当选择 hash 分区时填写，代表了 hash 分区的个数，其值必须是2的幂，范围在[2\^3，2\^20]

    集合只能存在于一个数据组中。

    格式：`Partition: <分区数>`

- AutoSplit ( *boolean* )：标识新集合是否开启自动切分功能，默认值为 false

    - 集合设置新的 hash 分区键后，可以使用该选项进行自动切分。
    - 不显式指定 AutoSplit 时，如果该集合修改前无指定 AutoSplit 且从属于某个非系统域，该域的 AutoSplit 参数将作用于此次设置。
    - 集合之前有指定 AutoSplit 为 false，需要显式设置 AutoSplit 为 true 进行自动切分。
    - AutoSplit 只能作用于 hash 分区键上。

    格式：`AutoSplit: true | false`

- EnsureShardingIndex ( *boolean* )：标识是否创建分区索引，默认值为 true

- Compressed ( *boolean* )：标识集合是否开启数据压缩功能

    如果设置 Compressed 为 true，而没有指定 CompressionType，则 CompressionType 为 "lzw"。

    格式：`Compressed: true | false`

- CompressionType ( *string* )：集合的压缩算法，"snappy" 或者 "lzw"

    - "snappy"：使用 snappy 算法压缩
    - "lzw"：使用 lzw 算法压缩

    格式：`CompressionType: "snappy" | "lzw"`

- StrictDataMode ( *boolean* )：标识对该集合的操作是否开启严格数据类型模式

    格式：`StrictDataMode: true | false`

- AutoIncrement ( *object* )：自增字段

    - option 中须加上 Field 属性，以标记要修改的字段。
    - 自增字段可以修改的属性有 CurrentValue, Increment, StartValue, MinValue, MaxValue, CacheSize, AcquireSize, Cycled, Generated。<br>属性具体功能请参考[自增字段介绍][sequence]。
    - 修改属性后，字段值将可能不唯一。如需保证修改后值唯一，建议使用唯一索引。

    格式：`AutoIncrement: <option>`
    
- AutoIndexId ( *Bool* )：标识是否创建 $id 索引，默认值是 true

    格式：`AutoIndexId: true | false`

    > **Note:**
    >
    > - 各个选项的具体使用方式见 [db.collectionspace.createCL()][createCL]。
    > - 分区集合不能修改与分区相关的属性，如 ShardingKey、Partition 等。
    > - EnsureShardingIndex 和 AutoSplit 仅对当前该次操作生效，仅当修改分区属性，如 ShardingKey 等时有效。

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`alter()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| ------ | -------- | -------------- | -------- |
| -32 | SDB_OPTION_NOT_SUPPORT | 选项暂不支持 | 检查当前集合属性，如果是分区集合不能修改与分区相关的属性|

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v1.12 及以上版本

##示例##

- 创建一个普通集合，然后将该集合修改为分区集合

    ```lang-javascript
    > db.sample.createCL('employee')
    > db.sample.employee.alter({ShardingKey: {a: 1}, ShardingType: "hash"})
    ```

- 创建一个普通集合，然后将该集合修改为分区集合，并且自动切分

    ```lang-javascript
    > db.sample.createCL('employee')
    > db.sample.employee.alter({ ShardingKey: {a: 1}, ShardingType: "hash", AutoSplit: true})
    ```

- 创建一个普通集合，然后将该集合修改为snappy压缩

    ```lang-javascript
    > db.sample.createCL('employee')
    > db.sample.employee.alter({CompressionType: 'snappy'})
    ```

- 创建一个有自增字段的集合，修改其自增起始值

    ```lang-javascript
    > db.sample.createCL('employee', {AutoIncrement: {Field: "studentID"}})
    > db.sample.employee.alter({AutoIncrement: {Field: "studentID", StartValue: 2017140000}})
    ```


[^_^]:
    本文使用的所有引用和链接
[setAttributes]:manual/Manual/Sequoiadb_Command/SdbCollection/setAttributes.md
[sequence]:manual/Distributed_Engine/Architecture/Data_Model/sequence.md
[createCL]:manual/Manual/Sequoiadb_Command/SdbCS/createCL.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[faq]:manual/FAQ/faq_sdb.md
[date_compression]:manual/Distributed_Engine/Architecture/compression_encryption.md
[consistency_strategy]:manual/Distributed_Engine/Architecture/Location/consistency_strategy.md

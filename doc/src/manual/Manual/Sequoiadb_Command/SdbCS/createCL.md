##名称##

createCL - 创建集合

##语法##

**db.collectionspace.createCL(\<name\>, [options])**

##类别##

SdbCS

##描述##

该函数用于在指定集合空间下创建集合（Collection）。集合是数据库中存放文档记录的逻辑对象，任何一条文档记录必须属于且仅属于一个集合。

##参数##

- name（ *string，必填* ）

    集合名

    - 在同一个集合空间中，集合名必须唯一。
    - 有关集合与集合命名的限制可参考[限制][sequoiadb_limitation]。

- options（ *object，选填* ）

    通过参数 options 可以设置集合的属性：

    - ShardingKey（ *object* ）：分区键，取值为 1 或 -1，表示正向或逆向排序

        格式：`ShardingKey: {<字段1>: <1|-1>, [<字段2>: <1|-1>, ...]}`

    - ShardingType（ *string* ）：分区方式，默认值为 "hash"
 
        取值如下：

        - "hash"：散列分区
        - "range"：范围分区

        格式：`ShardingType: "range"`

    - Partition（ *number* ）：分区数，默认值为 4096

        - 该参数取值必须是 2 的幂，范围在[2\^3，2\^20]。
        - 参数 ShardingType 的取值为"hash"时，该参数才能生效。

        格式：`Partition: 4096`

    - ReplSize（ *number* ）：写操作需同步的副本数，默认值为 1，表示写操作只需写入主节点

        取值如下：

        - -1：写请求需同步到该复制组若干活跃的节点之后，数据库写操作才返回应答给客户端
        - 0：写请求需同步到该复制组的所有节点之后，数据库写操作才返回应答给客户端
        - 1~7：写请求需同步到该复制组指定数量个节点之后，数据库写操作才返回应答给客户端

        格式：`ReplSize: 0`

    - ConsistencyStrategy（ *number* ）：[同步一致性][consistency_strategy]策略

        该参数用于指定数据同步优先选择的节点，默认值为 3。

        取值如下：

        - 1：节点优先策略
        - 2：位置多数派优先策略
        - 3：主位置多数派优先策略

        格式：`ConsistencyStrategy: 3`

    - Compressed（ *boolean* ）：是否开启数据压缩功能，默认值为 true，表示开启数据压缩功能

        格式：`Compressed: false`

    - CompressionType（ *string* ）：压缩算法类型，默认值为 "lzw"

        取值如下：

        - "snappy"：snappy 算法压缩
        - "lzw"：lzw 算法压缩

        格式：`CompressionType: "snappy"`

        >**Note:**
        >
        > snappy 压缩和 lzw 压缩的使用场景可参考[数据压缩][date_compression]。

    - AutoSplit（ *boolean* ）：是否开启自动切分功能，默认值为 false，不开启自动切分

        - 该参数不能与参数 Group 同时使用。
        - 参数 ShardingType 的取值为"hash"时，该参数才能生效。

        格式：`AutoSplit: true`

        >**Note:**
        >
        > 创建域和集合时均可指定参数 AutoSplit。如果显式指定集合的 AutoSplit，系统将优先按集合指定的值决定是否开启自动切分。

    - Group（ *string* ）：所属复制组

        - 该参数指定的值必须是集合所属域中包含的复制组。
        - 如果未指定该参数的值，集合将创建在集合所属域的任意一个复制组中。

        格式：`Group: "group1"`

    - AutoIndexId（ *boolean* ）：是否根据字段 _id 自动创建名为"$id"的唯一索引，默认值为 true，表示自动创建

        格式：`AutoIndexId: false`

    - EnsureShardingIndex（ *boolean* ）：是否根据参数 ShardingKey 指定的字段自动创建名为 "$shard" 的索引，默认值为 true，自动创建

        格式：`EnsureShardingIndex: false`

    - StrictDataMode（ *boolean* ）：是否开启严格数据类型模式，默认为 false，不开启

        开启严格模式后，如果数据类型为数值，在运算过程中出现溢出则会报错；如果数据类型非数值，则不进行任何操作。

      	格式：`StrictDataMode: true`

    - AutoIncrement（ *object* ）：自增字段，功能介绍可参考[自增字段][sequence]

        格式：`AutoIncrement: {Field: <字段名>, ...}` 或 `AutoIncrement: [{Field: <字段名1>, ...}, {Field: <字段名2>, ...}, ...]`
        
    - LobShardingKeyFormat（ *string* ）：指定大对象 ID 转换为分区键键值的转换格式

        目前仅支持对大对象 ID 中的时间属性进行转换，格式如下：
    
        - "YYYYMMDD"：转换为年月日的字符串形式，如 "20190701"
        - "YYYYMM"：转换为年月的字符串形式，如 "201907"
        - "YYYY"：转换为年的字符串形式，如 "2019"

        该参数仅在主集合中使用。当指定该参数时，必须保证参数 ShardingKey 指定的切分字段只有一个。
    
        格式：`LobShardingKeyFormat: "2021"`

    - IsMainCL（ *boolean* ）：是否为主集合，默认值为 false，不设置为主集合

        当指定集合为主集合时，子集合中的参数 ReplSize 和 AutoIncrement 会沿用主集合中对应参数的值，其他参数沿用自身参数的值。

        格式：`IsMainCL: true`

    - DataSource（ *string* ）：所使用的数据源名称

        格式：`DataSource: "ds1"`

    - Mapping（ *string* ）：所映射的集合名称

        格式：`Mapping: "employee"`

        >**Note:**
        >
        > 参数 DataSource 和 Mapping 的具体使用场景可参考[数据源][datasource]。

##返回值##

函数执行成功时，将返回一个 SdbCollection 类型的对象。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`createCL()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| ------ | -------- | -------------- | -------- |
| -2 | SDB_OOM | 无可用内存| 检查物理内存及虚拟内存的情况|
| -6 | SDB_INVALIDARG | 参数错误 | 查看参数是否填写正确|
| -22 | SDB_DMS_EXIST | 集合已存在| 检查集合是否存在|
| -34 | SDB_DMS_CS_NOTEXIST | 集合空间不存在| 检查集合空间是否存在|
| -318 | SDB_VALUE_OVERFLOW | 数值运算出现溢出| 检查运算过程是否存在溢出情况|

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v1.0 及以上版本

##示例##

- 在集合空间 sample 下创建集合 employee，不指定分区键

    ```lang-javascript
    > db.sample.createCL("employee")
    ```

- 在集合空间 sample 下创建集合 employee，指定分区键为字段 age、分区数为 4096 进行 hash 切分；指定参数 ReplSize 为 1，当发出写请求时，只需写入主节点即可返回

    ```lang-javascript
    > db.sample.createCL("employee", {ShardingKey: {age: 1}, ShardingType: "hash", Partition: 4096, ReplSize: 1})
    ```

- 在集合空间 sample 下创建集合 employee，指定开启严格数据类型模式

    ```lang-javascript
    > db.sample.createCL("employee", {StrictDataMode: true})
    ```
    
- 在主集合下使用大对象

    创建支持大对象的主集合 sample.maincl，将子集合 sample.subcl 挂载到该主集合上，并指定分区范围为 [20190701, 20190801)

    ```lang-javascript
    > db.sample.createCL("maincl", {LobShardingKeyFormat: "YYYYMMDD", ShardingKey: {date: 1}, IsMainCL: true, ShardingType: "range"})
    > db.sample.createCL("subcl")
    > db.sample.maincl.attachCL("sample.subcl", {LowBound: {date: "20190701"}, UpBound: {date: "20190801"}})
    ```

    创建大对象 ID 并指定时间属性

    ```lang-bash
    > db.sample.maincl.createLobID("2019-07-23-18.04.07")
    00005d36db97360002de8081
    ```

    指定大对象 ID，将文件 `/opt/data/test.dat` 以大对象形式插入集合 sample.maincl 中

    ```lang-bash
    > db.sample.maincl.putLob('/opt/data/test.dat', '00005d36db97360002de8081')
    00005d36db97360002de8081
    ```

    不指定大对象 ID，直接将文件 `/opt/data/test.dat` 以大对象形式插入集合 sample.maincl 中，插入成功后自动生成大对象 ID，其时间属性为当前时间

    ```lang-javascript
    > db.sample.maincl.putLob('/opt/data/test.dat')
    00005d36dbee370002de8080
    ```

[^_^]:
     本文使用的所有引用及链接
[sequence]:manual/Distributed_Engine/Architecture/Data_Model/sequence.md
[sequoiadb_limitation]:manual/Manual/sequoiadb_limitation.md#集合
[createCS]:manual/Manual/Sequoiadb_Command/Sdb/createCS.md
[domain]:manual/Distributed_Engine/Architecture/domain.md
[datasource]:manual/Distributed_Engine/Architecture/datasource.md
[Sequoiadb_error_code]:manual/Manual/Sequoiadb_error_code.md
[date_compression]:manual/Distributed_Engine/Architecture/compression_encryption.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[consistency_strategy]:manual/Distributed_Engine/Architecture/Location/consistency_strategy.md
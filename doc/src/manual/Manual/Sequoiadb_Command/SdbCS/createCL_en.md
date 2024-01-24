##NAME##

createCL - create a collection

##SYNOPSIS##

**db.collectionspace.createCL(\<name\>, [options])**

##CATEGORY##

SdbCS

##DESCRIPTION##

This function is used to create a collection in the specified collection space. Collection is a logical object that stores document records in a database. Any document record must belong to one and only one collection.

##PARAMETERS##

- name ( *string, required* )

    Collection name

    - In the same collection space, the collection name must be unique.
    - Refer to [Restrictions][sequoiadb_limitation] for restrictions on collections and collection naming.

- options ( *object, optional* )

    Modify the collection space properties through the parameter "options":

    - ShardingKey ( *object* ): Sharding key, and the value is 1 or -1, indicating forward or reverse sorting.

        Format: `ShardingKey: {<Field1>: <1|-1>, [<Field2>: <1|-1>, ...]}`

    - ShardingType ( *string* ): Sharding type, and the default value is "hash".
 
        The values are as follows:

        - "hash": Hash sharding
        - "range": Range sharding

        Format: `ShardingType: "range"`

    - Partition ( *number* ): The number of sharding, and the default value is 4096.

        - The value of this parameter must be a power of 2, and the range is [2\^3, 2\^20].
        - This parameter can only take effect when the value of the parameter "ShardingType" is "hash".

        Format: `Partition: 4096`

    - ReplSize ( *number* ): The number of replicas that need to be synchronized for write operations, and the default value is 1, which means that write operations only need to be written to the master node.

        The values are as follows:

        - -1: The write request needs to be synchronized to a number of active nodes in the replication group before the database write operation returns a response to the client.
        - 0: The write request needs to be synchronized to all nodes in the replication group before the database write operation returns a response to the client.
        - 1~7: The write request needs to be synchronized to the specified number of nodes in the replication group before the database write operation returns a response to the client.

        Format: `ReplSize: 0`

    - ConsistencyStrategy ( *number* ): [Synchronization consistency][consistency_strategy] strategy.

        This parameter is used to set the preferred node for data synchronization, the default value is 3.

        The values are as follows:

        - 1: Node priority strategy.
        - 2: Position majority first strategy.
        - 3: Main position majority first strategy.

        Format：`ConsistencyStrategy: 3`

    - Compressed ( *boolean* ): Whether to enable the data compression function, and the default value is "true", which means that the data compression function is enable.

        Format: `Compressed: false`

    - CompressionType ( *string* ): Compression algorithm type, and the default value is "lzw".

        The values are as follows:

        - "snappy": snappy algorithm compression
        - "lzw": lzw algorithm compression

        Format: `CompressionType: "snappy"`

        >**Note:**
        >
        > For the usage scenarios of "snappy" compression and "lzw" compression, please refer to [data compression][date_compression].

    - AutoSplit ( *boolean* ): Whether to enable the automatic segmentation function, and the default value is "false", and the automatic segmentation is not enabled.

        - This parameter cannot be used simultaneously with the parameter "Group".
        - This parameter can only take effect when the value of the parameter "ShardingType" is "hash".

        Format: `AutoSplit: true`

        >**Note:**
        >
        > User can specify the parameter AutoSplit when creating domains and collections. If user explicitly specify AutoSplit for a collection, the system will give priority to determining whether to enable automatic splitting according to the value specified by the collection.

    - Group ( *string* ): Collection replication group.

        - The value specified by this parameter must be the replication group contained in the domain to which the collection belongs.
        - If the value of this parameter is not specified, the collection will be created in any replication group of the domain to which the collection belongs.

        Format: `Group: "group1"`

    - AutoIndexId ( *boolean* ): Whether to automatically create a unique index named "$id" based on the field "_id", and the default value is "true", automatically created.
 
        Format: `AutoIndexId: false`

    - EnsureShardingIndex ( *boolean* ): Whether to automatically create an index named "$shard" according to the field specified by the parameter "ShardingKey", and the default value is "true", automatically created.

        Format: `EnsureShardingIndex: false`

    - StrictDataMode ( *boolean* ): Whether to enable strict data type mode, and the default is false, not enable.

        After enabling strict mode, if the data type is numeric, an error will be reported if an overflow occurs during the operation. if the data type is not numeric, no operation will be performed.

      	Format: `StrictDataMode: true`

    - AutoIncrement ( *object* ): Self-increment field, function introduction can refer to [Auto-increment field][sequence].

        Format: `AutoIncrement: {Field: <Field name>, ...}` 或 `AutoIncrement: [{Field: <Field name1>, ...}, {Field: <Field name2>, ...}, ...]`
        
    - LobShardingKeyFormat ( *string* ): Specify the conversion format of the LOB ID converted to the partition key key value.

        Currently, only the conversion of the time attribute in the LOB ID is supported, and the format is as follows:
    
        - "YYYYMMDD": Converted to the string form of year, month and day, such as "20190701".
        - "YYYYMM": Converted to the string form of year and month, such as "201907".
        - "YYYY": Converted to the string form of the year, such as "2019".

        This parameter is only used in the main collection. When specifying this parameter, user must ensure that there is only one segmentation field specified by the parameter "ShardingKey".
    
        Format: `LobShardingKeyFormat: "2021"`

    - IsMainCL ( *boolean* ): Whether it is the main collection, and the default value is "false", and it is not set as the main collection.

        When the designated collection is the main collection, the parameters "ReplSize" and "AutoIncrement" in the sub-collection will follow the values of the corresponding parameters in the main collection, and other parameters will follow the values of their own parameters.

        Format: `IsMainCL: true`

    - DataSource ( *string* ): The name of the data source used.

        Format: `DataSource: "ds1"`

    - Mapping ( *string* ): The name of the mapped collection.

        Format: `Mapping: "employee"`

        >**Note:**
        >
        > For the specific usage scenarios of the parameters "DataSource" and "Mapping", refer to [Data Source][datasource].

##RETURN VALUE##
 
When the function executes successfully, it will return an object of type SdbCollection.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `createCL()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -2 | SDB_OOM | No memory available| Check the physical memory and virtual memory.|
| -6 | SDB_INVALIDARG | Parameter error | Check whether the parameters are filled in correctly.|
| -22 | SDB_DMS_EXIST | Collection already exists| Check whether the collection exists.|
| -34 | SDB_DMS_CS_NOTEXIST | Collection space does not exist| Check whether the collection space exists.|
| -318 | SDB_VALUE_OVERFLOW | Numerical operation overflows| Check whether there is overflow in the operation process.|

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v1.0 and above

##EXAMPLES##

- Create the collection "employee" under the collection space "sample" without specifying the partition key.

    ```lang-javascript
    > db.sample.createCL("employee")
    ```

- Create the collection "employee" under the collection space "sample", specify the partition key as the field "age" and the number of partitions as 4096 for "hash" segmentation. Specify the parameter "ReplSize" as 1, when a write request is issued, only write to the master node to return.

    ```lang-javascript
    > db.sample.createCL("employee", {ShardingKey: {age: 1}, ShardingType: "hash", Partition: 4096, ReplSize: 1})
    ```

- Create the collection "employee" under the collection space "sample", and specify the strict data type mode to be turned on.

    ```lang-javascript
    > db.sample.createCL("employee", {StrictDataMode: true})
    ```
    
- Use LOB under the main collection

    Create the main collection "sample.maincl" that supports large objects, mount the sub-collection "sample.subcl" to the main collection, and specify the partition range as [20190701, 20190801).

    ```lang-javascript
    > db.sample.createCL("maincl", {LobShardingKeyFormat: "YYYYMMDD", ShardingKey: {date: 1}, IsMainCL: true, ShardingType: "range"})
    > db.sample.createCL("subcl")
    > db.sample.maincl.attachCL("sample.subcl", {LowBound: {date: "20190701"}, UpBound: {date: "20190801"}})
    ```

    Create a LOB ID and specify time attributes.

    ```lang-bash
    > db.sample.maincl.createLobID("2019-07-23-18.04.07")
    00005d36db97360002de8081
    ```

    Specify the LOB ID and insert the file `/opt/data/test.dat` into the collection "sample.maincl" as a LOB.

    ```lang-bash
    > db.sample.maincl.putLob('/opt/data/test.dat', '00005d36db97360002de8081')
    00005d36db97360002de8081
    ```

    Without specifying the LOB ID, directly insert the file `/opt/data/test.dat` into the collection "sample.maincl" in the form of a large object. After the insertion is successful, the LOB ID is automatically generated, and its time attribute is the current time.
    
    ```lang-javascript
    > db.sample.maincl.putLob('/opt/data/test.dat')
    00005d36dbee370002de8080
    ```

[^_^]:
     Links
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
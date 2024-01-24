##NAME##

alter - modify the properties of the collection

##SYNOPSIS##

**db.collectionspace.collection.alter(\<options\>)**

##CATEGORY##

SdbCollection

##DESCRIPTION##

This function is used to modify the properties of the collection. For more details, refer to [setAttributes\(\)][setAttributes].

##PARAMETERS##

options ( *object, required* )

Modify the collection properties through the options parameter:

- ReplSize ( *number* )：The number of replicas that need to be synchronized for write operations. The values are as follows:

    - -1: Write request needs to be synchronized after the active node of the replication group, and then the database write operation returns the response to the client.
    - 0: Write request needs to be synchronized after all nodes in the replication group, and then the database write operation returns the response to the client.
    - 1 ~ 7: Write request needs to be synchronized after the specified number of nodes in the replication group, and then the database write operation returns the response to the client.

    Format: `ReplSize: <number>`

- ShardingKey ( *object* ): Sharding key, the value is 1 or -1, indicating forward or backward sorting.

- ConsistencyStrategy ( *number* ): [Synchronization consistency][consistency_strategy] strategy.

    This parameter is used to set the preferred node for data synchronization, the default value is 3.

    The values are as follows:

    - 1: Node priority strategy.
    - 2: Position majority first strategy.
    - 3: Main position majority first strategy.

    Format：`ConsistencyStrategy: 3`

- ShardingKey ( *object* ): Sharding key, and the value is 1 or -1, indicating f orward or reverse sorting.

    "ShardingKey" can be modified when the collection only exists in one data group, or the collection does not have subcollections mounted.

    Format: `ShardingKey: {<field1>: <1|-1>, [<field2>: <1|-1>, ...]}`

- ShardingType ( *string* ): Partition method, the default value is "hash", and the values are as follows:
    - "hash": Hash partition
    - "range": Range partition

    The collection can only exist in one data group.

    Format: `ShardingType: "hash" | "range"`

- Partition ( *number* ): Number of partitions. It represents the number of hash partitions and is only filled in when selecting "hash" , The value must be a power of 2, and the range is [2\^3, 2\^20].

    The collection can only exist in one data group.

    Format: `Partition: <num>`

- AutoSplit ( *boolean* ): Identify whether the automatic segmentation function is enabled for the new collection.

    - The default value is false.
    - After setting a new hash partition key for the collection, users can use this option for automatic segmentation.
    - When AutoSplit is not specified explicitly, If "AutoSplit" is not specified before the collection is modified and the collection belongs to a non-system domain, the "AutoSplit" parameter of this domain will affect this setting.
    - Before the collection, "AutoSplit" is specified as false, user need to explicitly set AutoSplit to true for automatic segmentation.
    - "AutoSplit" can only work on "hash" partition keys.

    Format: `AutoSplit: true | false`

- EnsureShardingIndex ( *boolean* ): Identifies whether to create a partition index. The default value is true.

- Compressed ( *boolean* ): Identifies whether the collection is enabled for data compression.

    If "Compressed" is set to "true" and "CompressionType" is not specified, then "CompressionType" is "lzw".

    Format: `Compressed: true | false`

- CompressionType ( *string* ): The compression algorithm of the collection, "snappy" or "lzw".

    - "snappy": Using "snappy" algorithm to compress.
    - "lzw": Using "lzw" algorithm to compress.

    Format: `CompressionType: "snappy" | "lzw"`

- StrictDataMode ( *boolean* ): Identifies whether the operation of the collection enables strict data type mode.

    Format: `StrictDataMode: true | false`

- AutoIncrement ( *object* )：Auto-increment field.

    - "Field" attribute must be added to "option" to mark the field to be modified.
    - The properties that can be modified by the self-increment field are CurrentValue, Increment, StartValue, MinValue, MaxValue, CacheSize, AcquireSize, Cycled, Generated. <br>Specific attribute function can refet to [Auto-increment][sequence].
    - After modifying the attribute, the field value may not be unique. If users need to ensure that the modified value is unique, it is recommended to use a unique index.

    Format: `AutoIncrement: <option>`

    > **Note:**
    >
    > - The specific way of using each option can refers to [createCL()][createCL].
    > - The partition collection cannot modify the attributes related to the partition, such as "ShardingKey", "Partition".
    > - "EnsureShardingIndex" and "AutoSplit" are only effective for the current operation, and only effective when modifying partition properties, such as "ShardingKey".

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `alter()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -32 | SDB_OPTION_NOT_SUPPORT | Options are not currently supported. | Check the attributes of the current collection, if it is a partitioned collection, user cannot modify the attributes related to the partition.|

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v1.12 and above

##EXAMPLES##

- Create a normal collection, and then modify the collection to a partitioned collection.

    ```lang-javascript
    > db.sample.createCL('employee')
    > db.sample.employee.alter({ShardingKey: {a: 1}, ShardingType: "hash"})
    ```

- Create a normal collection, then modify the collection to a partitioned collection, and split it automatically.

    ```lang-javascript
    > db.sample.createCL('employee')
    > db.sample.employee.alter({ ShardingKey: {a: 1}, ShardingType: "hash", AutoSplit: true})
    ```

- Create a normal collection, and then modify the collection to "snappy" compression.

    ```lang-javascript
    > db.sample.createCL('employee')
    > db.sample.employee.alter({CompressionType: 'snappy'})
    ```

- Create a collection with auto-increment fields and modify its auto-increment starting value.

    ```lang-javascript
    > db.sample.createCL('employee', {AutoIncrement: {Field: "studentID"}})
    > db.sample.employee.alter({AutoIncrement: {Field: "studentID", StartValue: 2017140000}})
    ```


[^_^]:
    Links
[setAttributes]:manual/Manual/Sequoiadb_Command/SdbCollection/setAttributes.md
[sequence]:manual/Distributed_Engine/Architecture/Data_Model/sequence.md
[createCL]:manual/Manual/Sequoiadb_Command/SdbCS/createCL.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[faq]:manual/FAQ/faq_sdb.md
[date_compression]:manual/Distributed_Engine/Architecture/compression_encryption.md
[consistency_strategy]:manual/Distributed_Engine/Architecture/Location/consistency_strategy.md

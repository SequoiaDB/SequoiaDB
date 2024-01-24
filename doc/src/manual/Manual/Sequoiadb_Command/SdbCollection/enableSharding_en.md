##NAME##

enableSharding - modify the properties of the collection to turn on the sharding properties

##SYNOPSIS##

**db.collectionspace.collection.enableSharding(\<options\>)**

##CATEGORY##

SdbCollection

##DESCRIPTION##

This function is used to modify the properties of the collection to turn on the sharding properties.

##PARAMETERS##

options ( *object, required* )

Modify the collection properties through the options parameters:

- ShardingKey ( *object, required* ): Sharding key, the value is 1 or -1, indicating forward or backward sorting.

    - The existing "ShardingKey" will be modified to a new "ShardingKey".
    - The collection can only exist in one data group, or the collection is the main table without mounting sub-tables.

    Format: `ShardingKey: {<field1>: <1|-1>, [<field2>: <1|-1>, ...]}`

- ShardingType ( *string* ): Sharding mode, the default is "hash" sharding. The values are as follows:

    - "hash"：Hash sharding
    - "range"：Range sharding
    
    The collection can only exist in one data group.

    Format: `ShardingType: "hash" | "range"`

- `Partition` ( *number* ): It represents the number of hash partitions, which only be filled when selecting "hash". The default value is 4096.

    - The value must be a power of 2, and the range is [2\^3, 2\^20].
    - The collection can only exist in one data group.

    Format: `Partition: <number>`

        
    
- `AutoSplit` ( *boolean* ): Identifies whether the automatic segmentation function is turned on. The default value is false.

    - After setting a new "hash" partition key for the collection, users can use this option for automatic segmentation.
    - When AutoSplit is not specified explicitly, If "AutoSplit" is not specified before the collection is modified and the collection belongs to a non-system domain, the "AutoSplit" parameter of this domain will affect this setting.
    - The "AutoSplit" is specified as false before the collection. User need to explicitly set AutoSplit to true for automatic segmentation.
    - "AutoSplit" can only work on "hash" partition keys.

    Format: `AutoSplit: true | false`

- `EnsureShardingIndex` ( *boolean* )：Identifies whether to create a partition index and the default value is true.

> **Note:**
>
> - The specific way of using each option refers to [createCL()][createCL].
> - The partition collection cannot modify the attributes related to the partition.
> - "EnsureShardingIndex" and "AutoSplit" are only effective for the current operation, and only effective when modifying partition properties, such as "ShardingKey".

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `enableSharding()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -32 | SDB_OPTION_NOT_SUPPORT | Options are not currently supported | Check the attributes of the current collection, if it is a partitioned collection, users cannot modify the attributes related to the partition.|

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.10 and above

##EXAMPLES##

- Create a normal collection, and then modify the collection to a sharding collection.

    ```lang-javascript
    > db.sample.createCL('employee')
    > db.sample.employee.enableSharding({ShardingKey: {a: 1}, ShardingType: "hash"})
    ```

- Create a normal collection, then modify the collection to a sharding collection, and split it automatically. 

    ```lang-javascript
    > db.sample.createCL('employee')
    > db.sample.employee.enableSharding({ShardingKey: {a: 1}, ShardingType: "hash", AutoSplit: true})
    ```


[^_^]:
     Links
[createCL]:manual/Manual/Sequoiadb_Command/SdbCS/createCL.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
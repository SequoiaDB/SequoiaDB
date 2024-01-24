##NAME##

createDataSource - create data source

##SYNOPSIS##

**db.createDataSource(\<name\>, \<address\>, [user], [password], [type], [options])**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to create a data source to achieve cross-cluster data access.

##PARAMETERS##

- name ( *string, required* )

    The name of data source, which is unique in the same database.

- address ( *string, required* )

    "Addresses" are all or some of the cluster coordinator node addresses of SequoiaDB data source.

    >**Note:**
    >
    > - Multiple coordinator node addresses can be configured with commas (,). Users need to ensure that the number of addresses does not exceed seven, and all addresses point to the same cluster.
    > - The host name of the machine where the coordinator node address is located cannot be the same as that of the local machine.

- user ( *string, optional* )

    Name of the data source.

- password ( *string, optional* )

    Data source user password.

- type ( *string, optional* )

    Data source type, currently only supports sequoiadb.

- options ( *object, optional* )

    Other optional parameters can be set through the options parameter:

    - AccessMode ( *string* ): Configure access permissions for the data source, including reading and writing data, default is "ALL".

        The values are as follows:

        - "READ": Allow read-only operation.
        - "WRITE": Allow write operation.
        - "ALL" or "READ|WRITE": Allow all operations.
        - "NONE": It does not allow any operation.

        Format: `AccessMode: "READ"`

    - ErrorFilterMask ( *string* ): Configure error filtering for data operations on data sources, default is "NONE".

        The values are as follows:

        - "READ": Filter data read errors.
        - "WRITE": Filter data write errors.
        - "ALL" or "READ|WRITE": Filter all data read and write errors.
        - "NONE": Do not filter any errors.

        Format: `ErrorFilterMask: "READ"`

    - ErrorControlLevel ( *string* ): Configure the error level when performing unsupported data operations (such as DDL) on the mapping collection or collection space, default is "low".

        The values are as follows:

        - "high": Report an error and output an error message.
        - "low": Ignore unsupported data operations and do not execute.

        Format: `ErrorControlLevel: "low"`

    - TransPropagateMode ( *string* ): Configure transaction propagation mode on data source, default is "never".

        The values are as follows:

        - "never": Transaction operation is forbidden. Report an error and output an error message.
        - "notsupport": Transaction operation is not supported on data source. The operation will be converted to non-transactional and send to data source. It will be excluded from the transaction.

        Format: `TransPropagateMode: "never"`

    - InheritSessionAttr ( *boolean* ): Whether session between local coordinator and data source node inherits session attributes from local session on the coordinator, the default value is true.

        The supported attributes include PreferredInstance, PreferredInstanceMode, PreferredStrict, PreferredPeriod and Timeout. 

        Format: `InheritSessionAttr: true`

##RETURN VALUE##

When the function executes successfully, it will return an object of type SdbDataSource.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `createDataSource()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ------ | -------- | -------------- | -------- |
| -369 | SDB_CAT_DATASOURCE_EXIST | The specified data source already exists. | Check if there is a data source with the same name. |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.4.3 and above

##EXAMPLES##

Create a data source named "datasource" that only allows read-only operations.

```lang-javascript
> db.createDataSource("datasource", "192.168.20.66:50000", "", "", "SequoiaDB", {AccessMode: "READ"})
```


[^_^]:
    Links
[limitation]:manual/Manual/sequoiadb_limitation.md
[datasource]:manual/Distributed_Engine/Architecture/datasource.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md

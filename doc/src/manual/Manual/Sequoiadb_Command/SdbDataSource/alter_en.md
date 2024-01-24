##NAME##

alter - Modify the metadata information of the data source

##SYNOPSIS##

**SdbDataSource.alter(\<options\>)**

##CATEGORY##

SdbDataSource

##DESCRIPTION##

This function is used to modify metadata information such as the name, the connection address lists and the access permissions of the data source.

##PARAMETERS##

options ( *object，required* )

The metadata information of the data source can be modified through the options parameter:

1. Name (string): Data source name

 format: `Name: "datasource"`

2. Address (string): Coordinating node address of the data source cluster

 format: `Address:"sdbserver:11810"`

3. User (string): User name of data source

 format: `User: "DSuser"`

4. Password (string): Data source password corresponding to User

 format: `Password:"12345"`

5. AccessMode (string): Access permissions of data source

 The values are as follows:

   - "READ": Allow read-only operation
   - "WRITE": Allow write operation
   - "ALL"or "READ|WRITE":Allow all operations
   - "NONE": No operation allowed

 format: `AccessMode: "READ"`

6. ErrorFilterMask (string): Control error filtering of data operations on data sources

 The values are as follows:

   - "READ": Filter error of data read
   - "WRITE": Filter error of data write
   - "ALL"or "READ|WRITE": Filter errors of all data read and write
   - "NONE": Do not filter any errors

  format: `ErrorFilterMask: "READ"`

7. ErrorControlLevel (string): Configure the error level when performing unsupported data operations (such as DDL) on the mapping collection or collection space and the default is  "low".

  The values are as follows:

   - "high": Report an error and output an error message
   - "low": Ignore unsupported data operations and do not execute

  format: `ErrorControlLevel: "low"`

8. TransPropagateMode ( *string* ): Configure transaction propagation mode on data source, default is "never".

  The values are as follows:

  - "never": Transaction operation is forbidden. Report an error and output an error message.
  - "notsupport": Transaction operation is not supported on data source. The operation will be converted to non-transactional and send to data source. It will be excluded from the transaction.

  format: `TransPropagateMode: "never"`

9. InheritSessionAttr ( *boolean* ): Whether session between local coordinator and data source node inherits session attributes from local session on the coordinator. The supported attributes include. The default value is true.

  format: `InheritSessionAttr: true`


##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get the error message or use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the error code. For more details, refer to [Troubleshooting](troubleshooting/general/general_guide.md).

##VERSION##

v3.2.8 and above

##EXAMPLES##

1. Get a reference to the datasource datasource

    ```lang-javascript
    > var ds = db.getDataSource("datasource")
    ```

2. Modify the access permission of the data source to "WRITE"

    ```lang-javascript
    > ds.alter({AccessMode:"WRITE"})
    ```
##NAME##

setAttributes - modify the properties of the collection space

##SYNOPSIS##

**db.collectionspace.setAttributes(\<options\>)**

##CATEGORY##

SdbCS

##DESCRIPTION##

This function is used to modify the properties of the collection space.

##参数##

options ( *object, required* )

Modify the collection space properties through the options parameter:

- PageSize ( *number* ): Data page size, in bytes.

    - "PageSize" can only be one of 0, 4096, 8192, 16384, 32768, 65536.
    - When "PageSize" is 0, the default value is 65536.
    - Data cannot exist when modifying "PageSize".

    Format: `PageSize: <number>`

- LobPageSize ( *number* ): LOB data page size, in bytes.

    - "LobPageSize" can only be one of 0, 4096, 8192, 16384, 32768, 65536, 131072, 262144, 524288.
    - When "LobPageSize" is 0, the default value is 262144.
    - LOB data cannot exist when modifying "LobPageSize".

    Format: `LobPageSize: <number>`

- Domain ( *string* ): The domain of the collection space.

    The data of the collection space must be distributed on the group of the newly designated domain.

    Format: `Domain: <domain>`

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `setAttributes()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -32 | SDB_OPTION_NOT_SUPPORT | Options are not currently supported. | Check whether the current collection space properties are supported.|

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.10 and above

##EXAMPLES##

- Create a collection space with a data page size of 4096, and then modify the data page size of the collection space to 8192.

    ```lang-javascript
    > db.createCS('sample', {PageSize: 4096})
    > db.sample.setAttributes({PageSize: 8192})
    ```


- Create a collection space, and then specify a domain for the collection space.

    ```lang-javascript
    > db.createCS('sample')
    > db.sample.setAttributes({Domain: 'domain'})
    ```

[^_^]:
    Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md

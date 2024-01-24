##NAME##

createCS - create a collection space

##SYNOPSIS##

**db.createCS(\<name\>, [options])**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to create a collection space in a database object.

##PARAMETERS##

- name ( *string, required* )

    - Collection space name, users can refer to [limitation][limitation] for naming restrictions.
    - In the same database object, the collection space name must be unique.

- options ( *object, optional* )

    The properties of the collection space can be set through the options parameter:

    - PageSize ( *number* ): Specify the size of data page/index page. The default value is 65536 and the unit is byte.

        The value of this parameter can only be 0, 4096, 8192, 16384, 32768 or 65536, and a value of 0 indicates the default value is selected.

        Format: `PageSize: 4096`

    - Domain ( *string* ): Specify the domain. The default domain is the system domain "SYSDOMAIN", and it contains all replication groups.

        The domain specified by this parameter must already exist and cannot be specified as a system domain.
       
        Format: `Domain: "mydomain"`

    - LobPageSize ( *number* ): Specify the size of the Lob data page. The default value is 262144 and the unit is byte.

        The value of this parameter can only be 0, 4096, 8192, 16384, 32768, 65536, 131072, 262144 or 524288, and a value of 0 indicates the default value is selected.

        Format: `LobPageSize: 65536`

    - DataSource ( *string* ): Specify the name of the data source to be used.

        Format: `DataSource: "datasource"`

    - Mapping ( *string* ): Specify the name of the mapped collection space.

        Format: `Mapping: "sample"`

    > **Note:**
    >
    > - The parameters PageSize and LobPageSize cannot be modified after data is written in the collection space. Users should carefully select the values. 
    > - For specific usage scenarios of parameters DataSource and Mapping, refer to the [data source][datasource].
    > * In order to be compatible with earlier versions of the interface, `db.createCS(<name>, [PageSize])` is still available.


##RETURN VALUE##

When the function executes successfully, it will return an object of type SdbCS.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens，use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

* Create a collection space named "sample".

    ```lang-javascript
    > db.createCS("sample")
    ```


* Create a collection space named "sample". Specify the size of the data page as 4096B and the domain as "mydomain".

    ```lang-javascript
    > db.createCS("sample", {PageSize: 4096, Domain: "mydomain"})
    ```


[^_^]:
   links
[limitation]:manual/Manual/sequoiadb_limitation.md
[datasource]:manual/Distributed_Engine/Architecture/datasource.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
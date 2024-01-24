##NAME##

setGroups - modify the replication group contained in the domain

##SYNOPSIS##

**domain.setGroups(\<options\>)**

##CATEGORY##

SdbDomain

##DESCRIPTION##

This function is used to modify the replication group contained in the domain.

##PARAMETERS##

options ( *object, required* )

Modify the replication group contained in the domain through the parameter "options":

-  Groups ( *string/array* ): The replication group contained in the domain.

    When modifying the replication group contained in the domain, if it involves deleting the replication group, make sure that there is no data in the replication group, otherwise the operation will report an error.

    Format: `Groups: ['group1', 'group2']`


##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `setGroups()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -154   | SDB_CLS_GRP_NOT_EXIST |The replication group does not exist. | Use the list to check whether the replication group exists. |
| -256   |SDB_DOMAIN_IS_OCCUPIED | Domain has been used.   | Use [listCollectionSpaces()][listCollectionSpaces] to check whether there is a collection space in the domain. |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.4 and above

##EXAMPLES##

- Create a domain containing the replication group "group1" and "group2", no data exists in the replication groups.

    ```lang-javascript
    > var domain = db.createDomain('mydomain', ['group1', 'group2'])
    ```

    Modify the replication group contained in the domain to "group1" and group3.

    ```lang-javascript
    > domain.setGroups({Groups: ['group1', 'group3']})
    ```

- Create a domain containing the replication group "group1" with data in the replication group.

    ```lang-javascript
    > var domain = db.createDomain('mydomain', ['group1'])
    ```

    Modify the replication group contained in the domain to "group2".
 
    ```lang-javascript
    > domain.setGroups({Groups: ['group2']})
    ```

    Because there is data in "group1", the operation reports an error.
   
    ```lang-javascript
    (nofile):0 uncaught exception: -256
    Domain has been used
    ```

[^_^]:
     Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[listCollectionSpaces]:manual/Manual/Sequoiadb_Command/SdbDomain/listCollectionSpaces.md
[split]:manual/Manual/Sequoiadb_Command/SdbCollection/split.md
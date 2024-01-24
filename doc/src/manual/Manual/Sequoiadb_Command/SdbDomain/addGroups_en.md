##NAME##

addGroups - add a replication group in the domain

##SYNOPSIS##

**domain.addGroups(\<options\>)**

##CATEGORY##

SdbDomain

##DESCRIPTION##

This function is used to add a replication group in the domain.

>**Note:**
>
> The newly added replication group does not affect the data distribution and attributes of the original collection in the domain, but only affects the newly created collection.


##PARAMETERS##

options ( *object, required* )

The parameters of the replication group can be set through the options parameter:

- Groups ( *string/array* ): New replication group

    Format: `Groups: ['group1', 'group2']`


##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `addGroups()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -154   | SDB_CLS_GRP_NOT_EXIST|Partition group dose not exist| Use the list to check whether the partition group exists. |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

Create a domain containing two replication groups, and then turn on automatic split.

```lang-javascript
> var domain = db.createDomain('mydomain', ['group1', 'group2'], {AutoSplit: true})
```

* Add the replication group "group3" in the domain.

    ```lang-javascript
    > domain.addGroups({Groups: ['group3']})
    ```

* Add the replication groups "group4" and "group5" in the domain.

    ```lang-javascript
    > domain.addGroups({Groups: ['group4', 'group5']})
    ```  

[^_^]:
     Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
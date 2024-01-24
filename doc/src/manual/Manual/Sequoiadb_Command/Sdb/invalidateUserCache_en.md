##NAME##

invalidateUserCache - Clear User Privileges Cache on Nodes

##SYNOPSIS##

**db.invalidateUserCache( [username], [options])**

## CATEGORY ##

Sdb

## DESCRIPTION ##

This function is used to clear the user privileges cache on nodes.

## PARAMETERS ##

| Parameter | Type     | Description                                   | Required |
|-----------|----------|-----------------------------------------------|----------|
| username  | String   | The username.                                 | No       |
| options   | Json     | [Command Location Parameters][list_info] | No       |

> **Note:**
>
> When options are not specified, the scope is all coordination nodes, all data nodes, and all catalog nodes.

## RETURN VALUE ##

Upon successful execution, this function does not return anything.

Upon failure, it throws an exception and outputs an error message.

## ERRORS ##

When an exception is thrown, you can retrieve the error message using [getLastErrMsg()][getLastErrMsg] or the error code using [getLastError()][getLastError]. For more error handling, refer to the [Common Error Handling Guide][error_guide].

## VERSION ##

v5.8 and above

## EXAMPLES ##

* Clear the privileges cache of all users on all nodes.

    ```lang-javascript
    > db.invalidateUserCache()
    ```

* Clear the privileges cache of all users in group 'group1'.

    ```lang-javascript
    > db.invalidateUserCache("", { GroupName: 'group1' })
    ```

* Clear the privileges cache of a specific user in group 'group1'.

    ```lang-javascript
    > db.invalidateUserCache("myuser", { GroupName: 'group1' })
    ```

[^_^]:
    All references and links used in this document

[list_info]:manual/Manual/List/list.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md
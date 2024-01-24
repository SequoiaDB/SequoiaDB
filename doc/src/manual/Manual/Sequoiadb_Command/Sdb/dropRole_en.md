##NAME##

dropRole - Delete Custom Role

##SYNOPSIS##

**db.dropRole(\<rolename\>)**

## CATEGORY ##

Sdb

## DESCRIPTION ##

This function is used to delete [custom roles][user_defined_roles].

## PARAMETERS ##

rolename (*string, required*)

Specifies the name of the custom role to be deleted.

## RETURN VALUE ##

There is no return value when the function is executed successfully.

When the function execution fails, an exception will be thrown with an error message.

## ERRORS ##

Common exceptions are as follows:

| Error Code | Error Type | Description | Solution |
| ------ | ------ | --- | ------ |
| -409 | SDB_AUTH_ROLE_NOT_EXIST | The specified role does not exist | |

When an exception is thrown, you can retrieve the error message through [getLastErrMsg()][getLastErrMsg] or [getLastError()][getLastError] to get the [error code][error_code]. For more error handling, you can refer to the [Common Error Handling Guide][faq].

## VERSION ##

v5.8 and above

## EXAMPLE ##

Delete the role named `foo_developer` in the cluster.

```lang-javascript
> db.dropRole("foo_developer")
```

[^_^]:
    All references and links used in this document:
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[user_defined_roles]:manual/Distributed_Engine/Maintainance/Security/Role_Based_Access_Control/user_defined_roles.md
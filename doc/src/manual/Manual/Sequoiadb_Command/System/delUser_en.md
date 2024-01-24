
##NAME##

delUser - delete an operating system user

##SYNOPSIS##

**System.delUser(\<users\>)**

##CATEGORY##

System

##DESCRIPTION##

This function is used to delete an operating system users.

##PARAMETERS##

users ( *object, required* )

Parameter users can be used to set the user to be deleted:

- name ( *string* ): User name. This parameter is required.

    Format: `name: "username"`

- isRemoveDir ( *boolean* ): Whether to remove the user directory, the defual is false.

    Format: `isRemoveDir: true`

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens，use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.2 and above

##EXAMPLES##

Delete the specified system user.

```lang-javascript
> System.delUser({name: "newUser"})
```



[^_^]:
    links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[faq]:manual/FAQ/faq_sdb.md
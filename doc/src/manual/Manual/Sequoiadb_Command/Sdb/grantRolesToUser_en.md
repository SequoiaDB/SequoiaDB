##NAME##

grantRolesToUser - Add Roles to a User

##SYNOPSIS##

**db.grantRolesToUser(\<username\>, \<roles\>)**

## CATEGORY ##

Sdb

## DESCRIPTION ##

This function is used to add [custom roles][user_defined_roles] and [built-in roles][builtin_roles] to a user.

## PARAMETERS ##

| Parameter | Type       | Required | Description                                                  |
|-----------|------------|----------|--------------------------------------------------------------|
| username  | _string_   | Yes      | Specifies the name of the user to be updated.               |
| roles     | _array_    | Yes      | An array of roles to be added to the user.                   |

## RETURN VALUE ##

Upon successful execution, this function does not return any value.

Upon failure, it throws an exception and outputs an error message.

## ERRORS ##

Common exceptions are as follows:

| Error Code | Error Type | Description | Solution |
| ------ | ------ | --- | ------ |
| -409 | SDB_AUTH_ROLE_NOT_EXIST | The specified roles does not exist | |

When an exception is thrown, you can retrieve the error message using [getLastErrMsg()][getLastErrMsg] or the [error code][error_code] using [getLastError()][getLastError]. For more error handling, refer to the [Common Error Handling Guide][faq].

## VERSION ##

v5.8 and above

## EXAMPLES ##

Add built-in roles and custom roles to the user `user1` in the cluster.

```lang-javascript
> db.grantRolesToUser("user1",["_foo.admin","other_role"])
```

[^_^]:
    All references and links used in this document
[getLastErrMsg]: manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]: manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]: manual/FAQ/faq_sdb.md
[error_code]: manual/Manual/Sequoiadb_error_code.md
[builtin_roles]: manual/Distributed_Engine/Maintainance/Security/Role_Based_Access_Control/builtin_roles.md
[user_defined_roles]: manual/Distributed_Engine/Maintainance/Security/Role_Based_Access_Control/user_defined_roles.md
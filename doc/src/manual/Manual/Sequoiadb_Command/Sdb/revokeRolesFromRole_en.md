##NAME##

revokeRolesFromRole - Revoke Inherited Roles from Custom Role

##SYNOPSIS##

**db.revokeRolesFromRole(\<rolename\>, \<roles\>)**

## CATEGORY ##

Sdb

## DESCRIPTION ##

This function is used to revoke inherited custom roles and [built-in roles][builtin_roles] from a [custom role][user_defined_roles].

## PARAMETERS ##

| Parameter | Type      | Required | Description                                |
|-----------|-----------|----------|--------------------------------------------|
| rolename  | *string*  | Yes      | The name of the role to be updated.       |
| roles     | *array*   | Yes      | An array of inherited roles to revoke from the role. |

## RETURN VALUE ##

Upon successful execution, this function does not return anything.

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

In the cluster, revoke inherited built-in roles and other custom roles from the role named `foo_developer`.

```lang-javascript
> db.revokeRolesFromRole("foo_developer",["_foo.admin","other_role"])
```

[^_^]:
    All references and links used in this document
[getLastErrMsg]: manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]: manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]: manual/FAQ/faq_sdb.md
[error_code]: manual/Manual/Sequoiadb_error_code.md
[user_defined_roles]: manual/Distributed_Engine/Maintainance/Security/Role_Based_Access_Control/user_defined_roles.md
[builtin_roles]: manual/Distributed_Engine/Maintainance/Security/Role_Based_Access_Control/builtin_roles.md
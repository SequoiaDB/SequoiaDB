##NAME##

revokePrivilegesFromRole - Revoke Role Privileges

##SYNOPSIS##

**db.revokePrivilegesFromRole(\<rolename\>, \<privileges\>)**

## CATEGORY ##

Sdb

## DESCRIPTION ##

This function is used to revoke privileges from [custom roles][user_defined_roles].

## PARAMETERS ##

| Parameter | Type      | Required | Description                                |
|-----------|-----------|----------|--------------------------------------------|
| rolename  | *string*  | Yes      | The name of the role to be updated.       |
| privileges| *array*   | Yes      | An array of privileges to revoke from the role. Each privilege consists of a Resource and Actions. |

## RETURN VALUE ##

Upon successful execution, this function does not return anything.

Upon failure, it throws an exception and outputs an error message.

## ERRORS ##

Common exceptions are as follows:

| Error Code | Error Type | Description | Solution |
| ------ | ------ | --- | ------ |
| -409 | SDB_AUTH_ROLE_NOT_EXIST | The specified role does not exist | |

When an exception is thrown, you can retrieve the error message using [getLastErrMsg()][getLastErrMsg] or the [error code][error_code] using [getLastError()][getLastError]. For more error handling, refer to the [Common Error Handling Guide][faq].

## VERSION ##

v5.8 and above

## EXAMPLES ##

Create a role named `foo_developer` in the cluster, which inherits the built-in role `_foo.readWrite`, and additionally grants it the `snapshot` privilege on the cluster.

```lang-javascript
> db.revokePrivilegesFromRole("foo_developer",[
      {Resource:{cs: "foo", cl: ""}, Actions:["createCL", "dropCL"]}
   ])
```

[^_^]:
    All references and links used in this document
[getLastErrMsg]: manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]: manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]: manual/FAQ/faq_sdb.md
[error_code]: manual/Manual/Sequoiadb_error_code.md
[user_defined_roles]: manual/Distributed_Engine/Maintainance/Security/Role_Based_Access_Control/user_defined_roles.md
[builtin_roles]: manual/Distributed_Engine/Maintainance/Security/Role_Based_Access_Control/builtin_roles.md
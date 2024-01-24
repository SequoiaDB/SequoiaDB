##NAME##

updateRole - Update Role

##SYNOPSIS##

**db.updateRole(\<rolename\>, \<role\>)**

## CATEGORY ##

Sdb

## DESCRIPTION ##

This function is used to update [custom roles][user_defined_roles], which will overwrite the original definition of privileges and inherited roles.

## PARAMETERS ##

| Parameter | Type     | Required | Description                                                     |
|-----------|----------|----------|-----------------------------------------------------------------|
| rolename  | *string* | Yes      | The name of the role to be updated.                             |
| role      | *object* | Yes      | The role object containing the updated privileges and roles.    |
|           |          |          |   - Privileges (*array*)                                       |
|           |          |          |     An array of privileges to be granted to the role.           |
|           |          |          |     Each privilege consists of a Resource and Actions.          |
|           |          |          |   - Roles (*array*)                                            |
|           |          |          |     An array of roles from which the role inherits privileges.  |
|           |          |          |     It can include other custom roles or [built-in roles][builtin_roles]. |

## RETURN VALUE ##

Upon successful execution, this function does not return anything.

Upon failure, it throws an exception and outputs an error message.

## ERRORS ##

Common exceptions are as follows:

| Error Code | Error Type | Description | Solution |
| ------ | ------ | --- | ------ |
| -6   | SDB_INVALIDARG          | Invalid arguments | Check if the definition of privileges matches the schema |
| -409 | SDB_AUTH_ROLE_NOT_EXIST | The specified roles does not exist | |

When an exception is thrown, you can retrieve the error message using [getLastErrMsg()][getLastErrMsg] or the [error code][error_code] using [getLastError()][getLastError]. For more error handling, refer to the [Common Error Handling Guide][faq].

## VERSION ##

v5.8 and above

## EXAMPLES ##

In the cluster, update the role named `foo_developer` to inherit the built-in role `_foo.readWrite` and additionally grant it the `snapshot` privilege on the cluster.

```lang-javascript
> db.updateRole("foo_developer", {
   Privileges:[
      {Resource:{Cluster:true}, Actions:["snapshot"]}
   ],
   Roles:["_foo.readWrite"]
})
```

[^_^]:
    All references and links used in this document
[getLastErrMsg]: manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]: manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]: manual/FAQ/faq_sdb.md
[error_code]: manual/Manual/Sequoiadb_error_code.md
[user_defined_roles]: manual/Distributed_Engine/Maintainance/Security/Role_Based_Access_Control/user_defined_roles.md
[builtin_roles]: manual/Distributed_Engine/Maintainance/Security/Role_Based_Access_Control/builtin_roles.md
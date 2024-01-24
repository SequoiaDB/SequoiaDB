##NAME##

createRole - Create Role

##SYNOPSIS##

**db.createRole(\<role\>)**

## CATEGORY ##

Sdb

## DESCRIPTION ##

This function is used to create a [user-defined role][user_defined_roles].

## PARAMETERS ##

role (*object, required*)

Specify the role name, privileges, and inherited roles for the created role:

* Role (*string, required*): The name of the role. It cannot start with `_`.

* Privileges (*array*): An array of privileges granted to the role. Each privilege consists of a Resource and Actions.

* Roles (*array*): An array of roles from which this role inherits privileges. It can include other custom roles or [built-in roles][builtin_roles].

## RETURN VALUE ##

When the function executes successfully, there is no return value.

When the function execution fails, an exception will be thrown, and an error message will be displayed.

## ERRORS ##

Common exceptions are as follows:

| Error Code | Error Type | Description | Solution |
| ------ | ------ | --- | ------ |
| -6 | SDB_INVALIDARG | Invalid arguments | Check if the definition of role matches the schema |
| -408 | SDB_AUTH_ROLE_EXIST | A role with the same name already exists | |

When an exception is thrown, you can retrieve the error message using [getLastErrMsg()][getLastErrMsg] or get the [error code][error_code] using [getLastError()][getLastError]. For more error handling, refer to the [Common Error Handling Guide][faq].

## VERSION ##

v5.8 and above

## EXAMPLE ##

Create a role named `foo_developer` in the cluster, inheriting the built-in role `_foo.readWrite`, and additionally granting it the `snapshot` privilege on the cluster.

```javascript
> db.createRole({
   Role: "foo_developer",
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
[builtin_roles]: manual/Distributed_Engine/Maintainance/Security/Role_Based_Access_Control/builtin_roles.md
[user_defined_roles]: manual/Distributed_Engine/Maintainance/Security/Role_Based_Access_Control/user_defined_roles.md
##NAME##

createUsr - Create a database user to prevent illegal users from illegally operating the database.

##SYNOPSIS##

***db.createUsr( \<name\>, \<password\>, [options] )***

***db.createUsr( \<User\>, [options] )***

***db.createUsr( \<CipherUser\>, [options] )***

##CATEGORY##

Sdb

##DESCRIPTION##

Create a database user to prevent illegal users from illegally operating the database.

##PARAMETERS##

| Name       | Type     | Default | Description       | Required or not |
| ---------- | -------- | ------- | ----------------- | --------------- |
| name       | string   | ---     | username          | yes             |
| password   | string   | ---     | password          | yes             |
| User       | object   | ---     | [User](reference/Sequoiadb_command/AuxiliaryObjects/User.md) object       | yse             |
| CipherUser | object   | ---     | [CipherUser](reference/Sequoiadb_command/AuxiliaryObjects/CipherUser.md) object | yes             |
| options    | Json     | null    | extended options  | not             |

The detail description of 'options' parameter is as follow:

| Attributes | Type   | Description                       |
| ---------- | ------ | --------------------------------- |
| AuditMask  | string | The configuration mask of the user [auditlog][auditlog], the default value is "SYSTEM\|DDL\|DCL", and the values are as follows:<br>ACCESS, CLUSTER, SYSTEM, DCL, DDL, DML, DQL, INSERT, UPDATE, DELETE, OTHER, ALL, NONE<br>● Supports using 'bitwise or'(\|) to connect multiple masks, and 'logic not'(\!) prohibits a mask.<br>● A value of "ALL" indicates that all configuration masks are selected.<br>● A value of "NONE" indicates that all configuration masks are prohibited. That is, the audit function is turned off. |
| Role       | String | User role in old version. Currently only supports built-in roles in the system, and the value list: "admin", "monitor". "admin" is the administrator role, which can perform any operation. "monitor" is the monitoring role, which can only perform snapshot and list operations. |
| Roles      | Array  | User role list. You can grant multiple roles to users. For details, please refer to [Role-based Access Control][rbac] |

> **Note:**
>
> - This interface can only be used in cluster mode.
> - When a user is created in the database, the username and password must be specified to connect to the database.
> - For database username and password restrications, refer to [database limit][database_limit].

##RETURN VALUE##

On success, return void.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

1. Create a user with username 'sdbadmin' and password 'sdbadmin', and set the audit log mask.

	```lang-javascript
 	> db.createUsr( "sdbadmin", "sdbadmin", { AuditMask: "DDL|DML|!DQL" } )
 	```

2. Create a user with username 'sdbadmin' and password 'sdbadmin' using User object.

	```lang-javascript
 	> var a = User( "sdbadmin", "sdbadmin" )
    > db.createUsr( a )
 	```

3. Create a user with username 'sdbadmin' and password 'sdbadmin' using CipherUser object ( User information with username 'sdbadmin' and password 'sdbadmin' must exist in the cipher test file. For details on how to add and delete cipher test information in cipher test file, please see [sdbpasswd](database_management/tools/sdbpasswd.md) for details ).

    ```lang-javascript
    > var a = CipherUser("sdbadmin")
    > db.createUsr(a)
    ```

[^_^]:
     Links
[user]:manual/Manual/Sequoiadb_Command/AuxiliaryObjects/User.md
[cipherUser]:manual/Manual/Sequoiadb_Command/AuxiliaryObjects/CipherUser.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[passwd]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/sdbpasswd.md
[database_limit]:manual/Manual/sequoiadb_limitation.md#数据库
[auditlog]:manual/Distributed_Engine/Maintainance/DiagLog/auditlog.md
[rbac]: manual/Distributed_Engine/Maintainance/Security/Role_Based_Access_Control/Readme.md

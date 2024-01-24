##NAME##

dropUsr - Delete a database user.

##SYNOPSIS##

***db.dropUsr( \<name\>, \<password\> )***

***db.dropUsr( \<User\> )***

***db.dropUsr( \<CipherUser\> )***

##CATEGORY##

Sdb

##DESCRIPTION##

Delete a database user.

##PARAMETERS##

| Name       | Type     | Default | Description       | Required or not |
| ---------- | -------- | ------- | ----------------- | --------------- |
| name       | string   | ---     | username          | yes             |
| password   | string   | ---     | password          | yes             |
| User       | object   | ---     | [User](reference/Sequoiadb_command/AuxiliaryObjects/User.md) object       | yse             |
| CipherUser | object   | ---     | [CipherUser](reference/Sequoiadb_command/AuxiliaryObjects/CipherUser.md) object | yes             |

>Note：

>* When deleting a user, if there is no user other than the user to be deleted in the cluster who possesses the built-in role _root or the old version admin, the deletion will fail.

##RETURN VALUE##

On success, return void.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

1. Delete a user with username 'sdbadmin' and password 'sdbadmin'.

	```lang-javascript
 	> db.dropUsr( "sdbadmin", "sdbadmin" )
 	```

2. Delete a user with username 'sdbadmin' and password 'sdbadmin' using User object.

	```lang-javascript
 	> var a = User( "sdbadmin", "sdbadmin" )
 	> db.dropUsr( a )
	```

3. Delete a user with username 'sdbadmin' and password 'sdbadmin' using CipherUser object ( User information with username 'sdbadmin' and password 'sdbadmin' must exist in the cipher test file. For details on how to add and delete cipher test information in cipher test file, please see [sdbpasswd](database_management/tools/sdbpasswd.md) for details ).

	```lang-javascript
 	> var a = CipherUser( "sdbadmin" )
 	> db.dropUsr( a )
	```
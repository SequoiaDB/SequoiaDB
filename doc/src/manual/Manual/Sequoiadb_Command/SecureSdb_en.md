
##NAME##

SecureSdb - create a SecureSdb object

##SYNOPSIS##

***new SecureSdb( [hostname], [svcname] )***

***var securesdb = new SecureSdb( [hostname], [svcname], [username], [password] )***

##CATEGORY##

SecureSdb

##DESCRIPTION##

Create a SecureSdb object.

>**Note:**

>- SecureSdb is subclass of Sdb and SecureSdb object uses SSL connection.

>- The method and syntax of the SecureSdb object and the Sdb object are the same.

##PARAMETERS##

| Name     | Type   | Default            | Description  | Required or not |
| -------- | ------ | ------------------ | ------------ | --------------- |
| hostname | string | localhost          | IP address   | not             |
| svcname  | int    | local sdbcm's port | sdbcm's port | not             |
| username | string | empty   | username of sequoiadb   | not             |
| password | string | empty   | password of sequoiadb   | not             |

##RETURN VALUE##

When the function executes successfully, return a SecureSdb.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When exception happens, use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md) and use [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) to get error message. For more details, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##VERSION##

v2.0 and above

##EXAMPLES##

- Create a SecureSdb object.

	```lang-javascript
 	> var securesdb = new SecureSdb( "192.168.20.71", 11790 )
 	```

- Create a SecureSdb object with a username and password.

	```lang-javascript
 	> var securesdb = new SecureSdb("sdbserver1",11810,"sdbadmin","123")
 	```

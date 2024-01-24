##NAME##

Sdb - connection SequoiaDB object.

##SYNOPSIS##

**var db = new Sdb( [hostname], [svcname] )**

**var db = new Sdb( [hostname], [svcname], [username], [password] )**

**var db = new Sdb( [hostname], [svcname], [User] )**

**var db = new Sdb( [hostname], [svcname], [CipherUser] )**

##CATEGORY##

Sdb

##DESCRIPTION##

Create a Sdb object to connect to the SequoiaDB.

##PARAMETERS##

| Name       | Type     | Default   | Description       | Required or not |
| ---------- | -------- | --------- | ----------------- | --------------- |
| hostname   | string   | localhost | target hostname   | not             |
| svcname    | int      | 11810     | target svcname    | not             |
| username   | string   | null      | username          | not             |
| password   | string   | null      | password          | not             |
| User       | object   | ---       | [User](manual/Manual/Sequoiadb_Command/AuxiliaryObjects/User.md) object       | not             |
| CipherUser | object   | ---       | [CipherUser](manual/Manual/Sequoiadb_Command/AuxiliaryObjects/CipherUser.md) object | not             |

>**Note:**

>* Use [createUsr()](manual/Manual/Sequoiadb_Command/Sdb/createUsr.md) to create a user and set the password.

>* If the SequoiaDB has users, you must use username and password to create the Sdb object.

##RETURN VALUE##

On success, return an object of Sdb.

On error, exception will be thrown.

##ERRORS##

The exceptions of `Sdb()` are as below:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -15 | SDB_NETWORK | Network error. | Check the hostname and port are reachable. |
| -79 | SDB_NET_CANNOT_CONNECT| Unable to connect to the address | Check that the configuration information of the address, port and node are correct. |
| -104 | SDB_CLS_NOT_PRIMARY| Primary node does not exit | Check if the current replicaGroup has a node with "isPrimary" being "true". Start the node if there is a node that is not started in the current replicaGroup. |
| -250 | SDB_CLS_NODE_BSFAULT | The node is not in normal status | Check the node status of the current replicaGroup. Like check if the catalog node is started. |


When exception happen, use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md). For more detial, please reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##HISTORY##

Since v1.12

##EXAMPLES##

1. Create a Sdb object and connect to the SequoiaDB, using the default hostname and svcname.

	```lang-javascript
 	> var db = new Sdb()
 	```

2. Create a Sdb object and connect to the SequoiaDB on the specified host, "sdbserver1".

	```lang-javascript
 	> var db = new Sdb( "sdbserver1", 11810 )
	```

3. Create a Sdb object and connect to the SequoiaDB on the specified host using the specified username and password.

	```lang-javascript
 	> var db = new Sdb( "sdbserver1", 11810, "sdbadmin", "sdbadmin" )
	```

4. Create a Sdb object and connect to the SequoiaDB on the specified host using User object.

	```lang-javascript
    > var a = User( "sdbadmin" ).promptPassword()
    password:
    sdbadmin
 	> var db = new Sdb( "sdbserver1", 11810, a )
	```

5. Create a Sdb object and connect to the SequoiaDB on the specified host using CipherUser object ( User information with username 'sdbadmin' and password 'sdbadmin' must exist in the cipher test file. For details on how to add and delete cipher test information in cipher test file, please see [sdbpasswd](manual/Distributed_Engine/Maintainance/Mgmt_Tools/sdbpasswd.md) for details ).

   	```lang-javascript
    > var a = CipherUser( "sdbadmin" )
 	> var db = new Sdb( "sdbserver1", 11810, a )
	```
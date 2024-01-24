
##NAME##

createOM - Create sdbom service(SAC service) in target host of sdbcm.

##SYNOPSIS##

**oma.createOM(\<svcname\>,\<dbpath\>,[config])**

##CATEGORY##

Oma

##DESCRIPTION##

Create sdbom service(SAC service) in target host of sdbcm.

**Note:**

* A cluster can only belong a SequoiaDB management center, but a SequoiaDB management center can manage multiple clusters. Generally only create a sdbcm service process.

##DESCRIPTION##

* `svcname` ( *Int | String*， *Required* )

	The port of the node.

* `dbpath` ( *String*， *Required* )

	The node data directory.

* `config` ( *Object*， *Optional* )

	Node configuration information.

   | Common configuration | description | default value |
   | -------- | ---- | ------ |
   | httpname | Set sdbcm web port | svcname + 4 |
   | wwwpath  | Set sdbcm web path | Web directory of SequoiaDB installation path |

##RETURN VALUE##

On success, return an object of oma.

On error, exception will be thrown.

##ERRORS##

the exceptions of `createOM()` are as below:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -6 | SDB_INVALIDARG | Parameter error. | Check if the parameters are correct. |
| -145 | SDBCM_NODE_EXISTED | Node already exist. | Check if the node already exists. |

When error happens, use [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)
to get the error message or use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)
to get the error code. See [troubleshooting](manual/FAQ/faq_sdb.md) for more details.

##HISTORY##

since v2.0

##EXAMPLES##

1. Create and start a sdbcm process with the port 11789, http port 8000, web path /opt/sequoiadb/web.

	```lang-javascript
	> var oma = new Oma("localhost", 11790)
	> oma.createOM( "11780", "/opt/sequoiadb/database/sms/11780",
                             { "httpname": 8000, "wwwpath": "/opt/sequoiadb/web" } )
	> oma.startNode( 11780 )
    ```
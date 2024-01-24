
##NAME##

start - Start sdbcm service.

##SYNOPSIS##

**Oma.start( [options] )**

##CATEGORY##

Oma

##DESCRIPTION##

Start sdbcm service. In general, the interface is used to temporarily start a temporary sdbcm to complete some temporary tasks.

##PARAMETERS##

| Name     | Type     | Default | Description | Required or not |
| -------- | -------- | ------- | ----------- | --------------- |
| options  | JSON     | --- | Optional, see options parameter description  | not |

The detail description of 'options' parameter is as follow:

| Attributes | Type | Default | Format | Description |
| ---------- | ---- | ------- | ------ | ----------- |
| port       | Int / String | 11790 | { port:11790 } | Specify the port of sdbcm |
| alivetime  | Int / String | 300 | { alivetime:300 } | Service survival time in seconds  |
| standalone  | Bool | false | { standalone:false } | Whether to start in standalone mode |

>Note:

>1. A machine normally has only one sdbcm service, but you can start a temporary sdbcm service in standalone mode.

>2. The alivetime parameter is valid only when the standalone parameter is true, and when the alivetime ends, the temporary sdbcm service ends automatically.


##RETURN VALUE##

There is no return value. On error, exception will be thrown.

##ERRORS##

When exception happens, use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md) and use [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) to get error message. For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Start a temporary sdbcm service through standalone mode and specify port 11780, the temporary sdbcm service alive time is 5 minutes(300 seconds).

	```lang-javascript
    > Oma.start({ port:11780,standalone:true })
    Success: sdbcm(11780) is successfully start (28741)
	```
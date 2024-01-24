##NAME##

updateConf - update the specified node configuration

##SYNOPSIS##

**db.updateConf(\<config\>, [options])**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to update the specified node configuration. The configuration with the effective type of "online effective" will take effect immediately after the update; the configuration with the effective type of "restart effective" will take effect after restarting the node. For the effective type of each configuration can refer to [parameter description][parameter]. If you need to set unofficial configuration, must set the `Force` option to `true`.

##PARAMETERS##

| Name | Type| Description | Required or not |
| ---- | --- | ----------- | --------------- |
| config | object | [Node Configuration Parameters][parameter] | required |
| options| object | [Command location parameter] [location]<br>If this parameter is not specified, the update operation takes effect on all nodes by default. | not |

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.9 and above

##EXAMPLES##

- Update the parameter "diaglevel" of the "online effective" type, and specify the updated node as 11820.

    ```lang-javascript
    > db.updateConf({diaglevel: 3}, {ServiceName: "11820"})
    ```

- Update the parameter "numpreload" of the "restart effective" type, and specify the updated node as 11820.

    ```lang-javascript
    > db.updateConf({numpreload: 10}, {ServiceName: "11820"})
    ```

    If the following information is returned, the node needs to be restarted.

    ```lang-javascript
    (shell):1 uncaught exception: -322
    Some configuration changes didn't take effect:
    Config 'numpreload' require(s) restart to take effect.
    ```

- Forced to set unoffficial configuration.
	```lang-javascript
	// connect to coord
	> db = new Sdb( "localhost", 11810 )
	> db.updateConf( { aaa: 10 }, { Force: true } )
	```


[^_^]:
     Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[parameter]:manual/Distributed_Engine/Maintainance/Database_Configuration/parameter_instructions.md
[location]:manual/Manual/Sequoiadb_Command/location.md
##NAME##

deleteConf - delete the specified node configuration

##SYNOPSIS##

**db.deleteConf(\<config\>, [options])**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to delete the specified node configuration from the configuration file, and the deleted configuration will be restored to the default value. The configuration with the effective type of "online effective" will take effect immediately after deletion; the configuration with the effective type of "restart effective" will take effect after restarting the node. For the effective type of each configuration can refer to [parameter description][parameter].

##PARAMETERS##

| Name | Type| Description | Required or not |
| ---- | --- | ----------- | --------------- |
| config | object |[Node configuration parameters][parameter], contains configuration names and placeholders.<br>For example: {preferedinstance: 1, diaglevel: 1}, where 1 has no special meaning and only appears as a placeholder.| required  |
| options| object |[Command positional parameters][location]<br>If this parameter is not specified, the delete operation will take effect on all nodes by default.| not |


##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.6 and above

##EXAMPLES##

- Delete the parameter "diaglevel" of the "online effective" type, and specify the effective node as 11820.

    ```lang-javascript
    > db.deleteConf({diaglevel: 3}, {ServiceName: "11820"})
    ```

- Delete the parameter "numpreload" of type "restart to take effect", and specify the effective node as 11820.

    ```lang-javascript
    > db.deleteConf({numpreload: 1}, {ServiceName: "11820"})
    ```

    If the following information is returned, the node needs to be restarted.

    ```lang-javascript
    (shell):1 uncaught exception: -322
    Some configuration changes didn't take effect:
    Config 'numpreload' require(s) restart to take effect.
    ```

[^_^]:
     Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[parameter]:manual/Distributed_Engine/Maintainance/Database_Configuration/parameter_instructions.md
[location]:manual/Manual/Sequoiadb_Command/location.md
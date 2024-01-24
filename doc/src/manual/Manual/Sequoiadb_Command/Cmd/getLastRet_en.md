
##NAME##

getLastRet - Get the error code returned by the last command execution.

##SYNOPSIS##

***getLastRet()***

##CATEGORY##

Cmd

##DESCRIPTION##

Get the error code returned by the last command execution.

##PARAMETERS##

NULL

##RETURN VALUE##

On success, return the error code returned by the last command execution.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Create a Command object.

    ```lang-javascript
    > var cmd = new Cmd()
    ```

* Executed command(For more detial on Command Object executed command, please reference to [Cmd:run](manual/Manual/Sequoiadb_command/Cmd/run.md)).

    ```lang-javascript
    > cmd.run( "ls", "/opt/trunk/test" )
    test1
    test2
    test3
    ```

* Get the error code returned by the last command execution.

    ```lang-javascript
    > cmd.getLastRet()
    0
    ```

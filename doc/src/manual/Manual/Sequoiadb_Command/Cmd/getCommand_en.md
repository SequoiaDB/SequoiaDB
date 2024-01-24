
##NAME##

getCommand - Get the last executed command.

##SYNOPSIS##

***getCommand()***

##CATEGORY##

Cmd

##DESCRIPTION##

Get the last executed command.

##PARAMETERS##

NULL

##RETURN VALUE##

On success, return the last executed command.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md). For more detial, please reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

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

* Get the last executed command.

    ```lang-javascript
    > cmd.getCommand()
    ls /opt/trunk/test
    ```
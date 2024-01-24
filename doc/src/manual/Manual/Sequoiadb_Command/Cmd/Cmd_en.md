
##NAME##

Cmd - Create a Command object.

##SYNOPSIS##

***new Cmd() / [remoteObj](manual/Manual/Sequoiadb_Command/Remote/Remote.md).getCmd()***

##CATEGORY##

Cmd

##DESCRIPTION##

Create a Command object.

##PARAMETERS##

NULL

##RETURN VALUE##

On success, return void.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Create a Command object.

    ```lang-javascript
    > var cmd = new Cmd()
    ```

* Create a remote Command object.

    ```lang-javascript
    > var remoteObj = new Remote( "192.168.20.71", 11790 )
    > var cmd = remoteObj.getCmd()
    ```
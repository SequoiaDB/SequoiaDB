
##NAME##

start - Execute the command.

##SYNOPSIS##

***start( \<cmd\>, [args], [timeout], [useShell] )***

##CATEGORY##

Cmd

##DESCRIPTION##

Execute the Shell command in the background.

##PARAMETERS##

| Name     | Type     | Default | Description        | Required or not |
| -------- | -------- | ------- | ------------------ | --------------- |
| cmd      | string   | ---     | Shell command name | yes             |
| args     | string   | NULL    | command parameter  | not             |
| timeout  | int      | 0       | set timeout        | not             |
| useShell | int      | 1       | whether to use /bin/sh to parse and execute the command. Default use /bin/sh.  | not             |


##RETURN VALUE##

On success, return the tid of the command execution.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Create a Command object.

    ```lang-javascript
    > var cmd = new Cmd()
    ```

* Execute the command.

    ```lang-javascript
    > cmd.start( "ls", "/opt/trunk/test" )
    28340
    ```

* Get the result of the command execution.

    ```lang-javascript
    > cmd.getLastOut()
    test1
    test2
    test3 
    ```
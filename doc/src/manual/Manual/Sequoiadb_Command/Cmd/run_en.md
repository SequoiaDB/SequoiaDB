
##NAME##

run - Execute the command.

##SYNOPSIS##

***run( \<cmd\>, [args], [timeout], [useShell] )***

##CATEGORY##

Cmd

##DESCRIPTION##

Execute the Shell command.

##PARAMETERS##

| Name     | Type     | Default | Description        | Required or not |
| -------- | -------- | ------- | ------------------ | --------------- |
| cmd      | string   | ---     | Shell command name | yes             |
| args     | string   | NULL    | command parameter  | not             |
| timeout  | int      | 0       | set timeout        | not             |
| useShell | int      | 1       | whether to use /bin/sh to parse and execute the command. Default use /bin/sh.  | not             |


##RETURN VALUE##

On success, return the resultof the command execution.

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
    > cmd.run( "ls", "/opt/trunk/test" )
    test1
    test2
    test3
    ```


##NAME##

runJS - Execute the JavaScript code remotely.

##SYNOPSIS##

***runJS( \<code\> )***

##CATEGORY##

Cmd

##DESCRIPTION##

Execute the JavaScript code remotely.

>**Note:** 

>runJS() should be called by remote Command obj

##PARAMETERS##

| Name      | Type     | Default | Description     | Required or not |
| --------- | -------- | ------- | --------------- | --------------- |
| code      | string   | ---     | JavaScript code | yes             |

##RETURN VALUE##

On success, return code execution result.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Create a remote Command object.( For more detial on establishing a remote connection,please reference to [Remote](manual/Manual/Sequoiadb_command/Remote/Remote.md) )

    ```lang-javascript
    > var remoteObj = new Remote( "192.168.20.71", 11790 )
    > var cmd = remoteObj.getCmd()
    ```

* Execute the JavaScript code remotely.

    ```lang-javascript
    > cmd.runJS( "1+2*3" )
    7
    ```

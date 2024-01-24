
##NAME##

getInfo - Get the object information of the Command object.

##SYNOPSIS##

***getInfo()***

##CATEGORY##

Cmd

##DESCRIPTION##

Get the object information of the Command object.

##PARAMETERS##

NULL

##RETURN VALUE##

On success, return the object information of the Command object.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Create a remote Command object.(For more detial on establishing a remote connection,please reference to [Remote](manual/Manual/Sequoiadb_command/Remote/Remote.md))

    ```lang-javascript
    > var remoteObj = new Remote( "192.168.20.71", 11790 )
    > var cmd = remoteObj.getCmd()
    ```

* Get the object information of the Command object.

    ```lang-javascript
    > cmd.getInfo()
    {
      "type": "Cmd",
      "hostname": "192.168.20.71",
      "svcname": "11790",
      "isRemote": true
    }
    ```
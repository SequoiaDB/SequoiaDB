
##NAME##

getCmd - Create a remote Command object.

##SYNOPSIS##

***remoteObj.getCmd()***

##CATEGORY##

Remote

##DESCRIPTION##

Create a remote Command object.

##PARAMETERS##

NULL

##RETURN VALUE##

On success, return void.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Create a remote object.

```lang-javascript
> var remoteObj = new Remote( "192.168.20.71", 11790 )
```

* Create a remote Command object. (For more detail about create Command object, please reference to [Cmd](manual/Manual/Sequoiadb_Command/Cmd/Cmd.md))

```lang-javascript
> var cmd = remoteObj.getCmd()
```

* Execute the JavaScript code remotely. (For more detial, please reference to [runJS](manual/Manual/Sequoiadb_Command/Cmd/runJS.md))

```lang-javascript
> cmd.runJS( "1+2*3" )
7
```
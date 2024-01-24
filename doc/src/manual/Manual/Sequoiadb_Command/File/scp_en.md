
##NAME##

scp - Remote copy file.

##SYNOPSIS##

***File.scp( \<srcFile\>, \<dstFile\>, \[isreplace\], \[mode\] )***

##CATEGORY##

File

##DESCRIPTION##

Remote copy file.

##PARAMETERS##

| Name    | Type     | Defaults | Description                     | Required or not |
| ------- | -------- | -------- | ------------------------------- | --------------- |
| srcFile | string   | ---      | source file path                | yes             |
| dstFile | string   | ---      | destination file path           | yes             |
| replace | boolean  | false    | whether replace the source file | not             |
| mode    | int      | 0644     | set file permissions            | not             |

The specific format of parameters srcFile and dstFile are as follows:

```lang-javascript
ip:sdbcmPort@filepath
hostname:sdbcmPort@filepath
```

>**Note:**
>
>The port refers to the port of the sdbcm. If the ip or hostname and sdbcmport in the parameters srcFile and desFile refer to the client's ip or hostname and sdbcmport, you can ignore them. 

##RETURN VALUE##

On success, return void.

On error, exception will be thrown.

##ERRORS##

when exception happens, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

Copy file from local host to remote host.

```lang-javascript
> File.scp( "/opt/sequoiadb/srcFile.txt", "192.168.20.71:11790@/opt/sequoiadb/desFile.txt" )
Success to copy file from /opt/sequoiadb/srcFile.txt to 192.168.20.71:11790@/opt/sequoiadb/desFile.txt
```
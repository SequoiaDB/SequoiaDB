
##NAME##

isProcExist - Determine if a process exists

##SYNOPSIS##

***System.isProcExist( \<options\> )***

##CATEGORY##

System

##DESCRIPTION##

Determine if a process exists

##PARAMETERS##

| Name      | Type     | Default | Description         | Required or not |
| ------- | -------- | ------------ | ---------------- | -------- |
| options | JSON   | ---    |  process information  | yes   |

The detail description of 'options' parameter is as follow:

| Attributes | Type    | Required or not | Format  | Description         |
| ---------- | ------- |---------------- | ------- | ---------------- |
| value   | string |  yes   | { value: "31831" }  | value of the specified type |
| type    | string | not  | if { type: "pid" }, then it means that determine if a process exists base on process id. if { type: "name" }, then it means that determine if a process exists base on service name    | specified type |

##RETURN VALUE##

On success, return true if the process exists, otherwise return false.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Judging by the specified pid：

    ```lang-javascript
    > System.isProcExist( { value: "31831", type: "pid" } )
    true
    ```

* Judging by the specified service name：

    ```lang-javascript
    > System.isProcExist( { value: "sdbcm(11790)", type: "name" } )
    true
    ```
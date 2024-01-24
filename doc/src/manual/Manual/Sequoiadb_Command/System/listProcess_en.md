
##NAME##

listProcess - List the information of processes

##SYNOPSIS##

***System.listProcess( \[options\], \[filter\] )***

##CATEGORY##

System

##DESCRIPTION##

List the information of processes

##PARAMETERS##

| Name      | Type     | Default | Description              | Required or not |
| --------- | -------- | ------ | -----------------------    | -------- |
| options   | JSON     |  no details are displayed by default | display pattern | not       |
| filter    | JSON     | display all processes by default | filter   | not      |

The detail description of 'options' parameter is as follow:

| Attributes | Type    | Required or not | Format  | Description         |
| ---------- | ------- |---------------- | ------- | -------------- |
| detail    | Bool |   not   | { detail: true }     | whether to display details   |

**Note:**

The optional parameter filter supports the AND, the OR, the NOT and exact matching of some fields in the result, and the result set is filtered.

##RETURN VALUE##

On success, return the information of processes.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* List the information of processes

    ```lang-javascript
    > System.listProcess()
    {
      "pid": "30571",
      "cmd": "sequoiadb(50000) S"
    }
    {
      "pid": "30834",
      "cmd": "bin/sdb"
    }
    {
      "pid": "30876",
      "cmd": "/usr/sbin/rsyslogd -n"
    }
    ...
    ```

* Filter the results

    ```lang-javascript
    > System.listProcess( { detail: true }, { "user": "sdbadmin" } )
    {
      "user": "sdbadmin",
      "pid": "20630",
      "status": "S",
      "cmd": "sleep 1"
    }
    {
      "user": "sdbadmin",
      "pid": "25681",
      "status": "Sl",
      "cmd": "sdbom(11780)"
    }
    ```

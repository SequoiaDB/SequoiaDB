
##NAME##

listLoginUsers - List the information of logged-in users

##SYNOPSIS##

***System.listLoginUsers( \[options\], \[filter\] )***

##CATEGORY##

System

##DESCRIPTION##

List the information of logged-in users

##PARAMETERS##

| Name      | Type     | Default | Description              | Required or not |
| --------- | -------- | ------ | -----------------------   | -------- |
| options   | JSON     | no details are displayed by default | display pattern  | not       |
| filter    | JSON     | display all logged-in users by default | filter, display all by default          | not      |

The detail description of 'options' parameter is as follow:

| Attributes | Type    | Required or not | Format  | Description         |
| ---------- | ------- |---------------- | ------- | -------------- |
| detail    | Bool |   not   | { detail: true }     | whether to display details   |

**Note:**

The optional parameter filter supports the AND, the OR, the NOT and exact matching of some fields in the result, and the result set is filtered.

##RETURN VALUE##

On success, return the information of logged-in users.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* List the information of all logged-in users

    ```lang-javascript
    > System.listLoginUsers()
    {
      "user": "sequoiadb"
    }
    {
      "user": "username"
    }
    ...
    ```

* Filter the results:
    
    ```lang-javascript
    > System.listLoginUsers( { detail: true }, { "tty": "tty1" } )
    {
      "user": "sequoiadb",
      "time": "2019-05-10 18:37",
      "from": "",
      "tty": "tty1"
    }
    ```

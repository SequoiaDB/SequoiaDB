
##NAME##

listAllUsers - List the information of users

##SYNOPSIS##

***System.listAllUsers( \[options\], \[filter\] )***

##CATEGORY##

System

##DESCRIPTION##

List the information of users

##PARAMETERS##

| Name      | Type     | Default | Description                 | Required or not |
| --------- | -------- | ----------------- | ----------------------  | -------- |
| options   | JSON     | no details are displayed by default | display pattern | not |
| filter    | JSON     | display all users by default | filter          | not      |

The detail description of 'options' parameter is as follow:

| Attributes | Type    | Required or not | Format  | Description         |
| ---------- | ------- |---------------- | ------- | -------------- |
| detail      | Bool    |   not         | { detail: true } | whether to display details   |

**Note:**

The optional parameter filter supports the AND, the OR, the NOT and exact matching of some fields in the result, and the result set is filtered.

##RETURN VALUE##

On success, return the information of users.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* List the information of all users

    ```lang-javascript
    > System.listAllUsers( )
    {
      "user": "sequoiadb"
    }
    {
      "user": "sdbadmin"
    }
    ...
    ```

* Filter the results:

    ```lang-javascript
    > System.listAllUsers( { detail: true }, { "user": "sequoiadb" } )
    {
      "user": "sequoiadb",
      "gid": "1000",
      "dir": "/home/sequoiadb"
    }
    ```


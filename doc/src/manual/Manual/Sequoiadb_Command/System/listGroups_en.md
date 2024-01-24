
##NAME##

listGroups - List the information of user groups

##SYNOPSIS##

***System.listGroups( \[options\], \[filter\] )***

##CATEGORY##

System

##DESCRIPTION##

List the information of user groups

##PARAMETERS##

| Name      | Type     | Default           | Description          | Required or not |
| --------- | -------- | ----------------- | -----------------------------  | -------- |
| options   | JSON     | no details are displayed by default | display pattern | not       |
| filter    | JSON     | display all groups by default | filter | not |

The detail description of 'options' parameter is as follow:

| Attributes | Type | Required or not | Format     | Description         |
| ---------- | ----- |---------------- | ---------- | -------------- |
| detail     | Bool |   not         | { detail: true } | whether to display details   |

**Note:**

The optional parameter filter supports the AND, the OR, the NOT and exact matching of some fields in the result, and the result set is filtered.

##RETURN VALUE##

On success, return the information of user groups.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* List the information of all user groups

    ```lang-javascript
    > System.listGroups()
    {
      "name": "sequoiadb"
    }
    {
      "name": "lpadmin"
    }
    {
      "name": "sambashare"
    }
    ...
    ```

* Filter the results

    ```lang-javascript
    > System.listGroups( { detail: true }, { "name": "sequoiadb" } )
    {
      "name": "sequoiadb",
      "gid": "1000",
      "members": ""
    }
    ```



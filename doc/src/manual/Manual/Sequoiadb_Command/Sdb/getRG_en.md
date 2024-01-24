##NAME##

getRG - get the specified replication group

##SYNOPSIS##

**db.getRG(\<name\>|\<id\>)**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to get the specified replication group.

##PARAMETERS##

|Name      |type        |Description  |Required or not |
|--------- |----------- |------------ |----------|
| name | string | Replication group name. In the same database object, the replication group name is unique. | Either name or id. |
| id | number | The ID of the replication group. Which is automatically assigned by the system when the replication group is created.  | Either name or id. |

> **Note:**
>
> The value of the name field cannot be an empty string, cannot contain dots(.) or dollar signs($), and the length does not exceed 127B.

##RETURN VALUE##

When the function executes successfully, it will return an object of type SdbReplicaGroup. 

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

* Specify the name value and return the reference of the replication group "group1".

    ```lang-javascript
    > db.getRG("group1")
    ```

* Specify the id value and return the reference of the replication group "group1"(assuming that the replication group ID of "group1" is 1000).

    ```lang-javascript
    > db.getRG(1000)
    ```


[^_^]:
   links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
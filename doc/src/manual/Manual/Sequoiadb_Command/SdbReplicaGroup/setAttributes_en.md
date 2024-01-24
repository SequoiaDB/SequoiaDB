##NAME##

setAttributes - set the attributes of a replication group

##SYNOPSIS##

**rg.setAttributes(\<options\>)**

##CATEGORY##

SdbReplicaGroup 

##DESCRIPTION##

This function is used to modify the attributes of the current replication group.

##PARAMETERS##

options ( *object, required* )

Modify the replication group attributes through the parameter "options":

- ActiveLocation ( *string* ): The location corresponding to "ActiveLocation".

    - The specified location needs to exist in the current replication group.
    - When the value is an empty string, it means to delete the "ActiveLocation" of the current replication group.

    Format: `ActiveLocation: "GuangZhou"`

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.6.1 and above

##EXAMPLES##

- Set location "GuangZhou" to "ActiveLocation" of replication group "group1".

    ```lang-javascript
    > var rg = db.getRG("group1")
    > rg.setAttributes({ActiveLocation: "GuangZhou"})
    ```

- Delete "ActiveLocation" from replication group "group1".

    ```lang-javascript
    > var rg = db.getRG("group1")
    > rg.setAttributes({ActiveLocation: ""})
    ```

[^_^]:
    Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
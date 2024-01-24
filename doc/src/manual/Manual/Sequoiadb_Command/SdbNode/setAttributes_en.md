##NAME##

setAttributes - modify the properties of a node

##SYNOPSIS##

**node.setAttributes(\<options\>)**

##CATEGORY##

SdbNode

##DESCRIPTION##

This function is used to modify the properties of the current node.

##PARAMETERS##

options ( *object, required* )

Modify the node properties through the parameter "options":

- Location ( *string* ): Node location information.

    When the value of this parameter is an empty string, it means to delete the position information of the current node.

    Format: `Location: "GuangZhou"`

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.6.1 and above

##EXAMPLES##

- Modify the location information of node 11820 to "GuangZhou".

    ```lang-javascript
    > var node = db.getRG("group1").getNode("hostname", 11820)
    > node.setAttributes({Location: "GuangZhou"})
    ```

- Delete the location information of the node.

    ```lang-javascript
    > node.setAttributes({Location: ""})
    ```

[^_^]:
    Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
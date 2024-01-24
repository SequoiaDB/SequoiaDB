##NAME##

setLocation - modify the location information of a node

##SYNOPSIS##

**node.setLocation(\<location\>)**

##CATEGORY##

SdbNode

##DESCRIPTION##

This function is used to modify the position information of the current node.

##PARAMETERS##

location ( *string, required* )

Node location information.

- The maximum length of location information is limited to 256 bytes.
- When the value is an empty string, it means to delete the location information of the current node.

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `setLocation()` function are as follows:

| Error Code | Error Type | Description | Solution |
| -------- | -------- | -------------- | -------- |
|-6|SDB_INVALIDARG|Parameter type error.|Check whether the parameter type is correct.|
|-259|SDB_OUT_OF_BOUND|Required parameter not specified.|Check whether required parameters are missing.|

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.6.1 and above

##EXAMPLES##

- Modify the location information of node 11820 to "GuangZhou".

    ```lang-javascript
    > var node = db.getRG("group1").getNode("hostname", 11820)
    > node.setLocation("GuangZhou")
    ```

- Delete the location information of the node.

    ```lang-javascript
    > node.setLocation("")
    ```

[^_^]:
    Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
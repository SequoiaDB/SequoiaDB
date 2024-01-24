##NAME##

connect - get the connection for the current node

##SYNOPSIS##

**node.connect([useSSL])**

##CATEGORY##

SdbNode

##DESCRIPTION##

Get the connection for the current node, and then performs a series of operations on the current node.
Use node.connect().help() to view the operations supported by this connection.

##PARAMETERS##

* useSSL ( *boolean*， *optional* )

  Whether to use SSL connection, default value is false.

> **Note:**

> 1. Only the Enterprise version supports SSL connection.

> 2. Need to set the database configuration item --usessl=true before use SSL connection.

##RETURN VALUE##

When the function executes successfully: return an object of [Sdb][Sdb].

When the function fails: an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `connect()` function are as follows:

| Error Code | Error Type | Description | Solution |
|------|--------|--------------|--------|
|-15   | SDB_NETWORK |  Network error |  Check the syntax, check whether the node is up |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the error code. For more details, refer to [Troubleshooting][faq].

##VERSION##

v1.10 and above

##EXAMPLES##

* Connect to current node.

```lang-javascript
> node.connect()
localhost:11820
```

* Connect to current node with SSL.

```lang-javascript
> node.connect(true)
localhost:11820
```

[^_^]:
     links
[Sdb]:manual/Manual/Sequoiadb_Command/Sdb/Sdb.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[faq]:manual/FAQ/faq_sdb.md

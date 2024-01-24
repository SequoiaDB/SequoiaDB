##NAME##

listNodes - list node information

##SYNOPSIS##

**oma.listNodes(\[options\], \[filter\])**

##CATEGORY##

Oma

##DESCRIPTION##

This function is used to list the eligible node information of the current sdbcm machine. It displays the information of data node, coord node and catalog node by default.

##PARAMETERS##

- options ( *object, optional* )

    Specify the node type, mode and other parameters through the parameter "options":

    - type ( *string* ): Node type, and the default is "db"
  
        The values are as follows:

        - "all": All nodes
        - "db": data node, coord node and catalog node
        - "om": sdbom node
        - "cm": sdbcm node

        Format: `type: "all"` 

    - mode ( *string* ): Node mode, and the default is "run"

        The values are as follows:

        - "run": Running node
        - "local": Local node, whether it is running or not

        Format: `mode: "local"`

    - role ( *string* ): Node role

        The values are as follows:

        - "data": data node
        - "coord": coord node
        - "catalog": catalog node
        - "standalone": standalone node
        - "om": sdbom node
        - "cm": sdbcm node

        Format: `role: "data"`

    - svcname ( *string* ): Node port number

        When specifying multiple svcnames, separate them with a comma (,).

        Format: `svcname: "11820, 11830" `

    - showalone ( *boolean* )：Whether to display the sdbcm node information started in [standalone mode][standalone], and the default is false.

        Format: `standalone: true`

    - expand ( *boolean* ): Whether to display the extended information of the node, and the default is false.

        Format: `expand: true`

- filter ( *object, optional* )

    Specify the conditions for filtering node information. It supports to retrieve the node information through [matching symbol][match] $and, $or, $not or exact match.

##RETURN VALUE##

When the function executes successfully, it will return an object of type BSONArray. Users can get a list of node details through this object. Field descriptions are as follows:

|Name|Type|Description |
|------|----|----|
|svcname|string|Node port number|
|type|string|Node type|
|role|string|Node role|
|pid|int32|Process ID|
|groupid|int32|Node's replication group ID|
|nodeid|int32|Node ID|
|primary|int32|Whether the node is the primary node. 1 means the primary node and 0 means the secondary node.|
|isalone|int32|Whether the node is started in standalone mode (only valid when the parameter "role" is "cm").|
|groupname|string|Node's replication group name|
|starttime|string|Node startup time |
|dbpath|string|Path to store data files|

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

Connect to the local cluster management service process sdbcm to obtain information of the node 11830.

```lang-javascript
> var oma = new Oma("localhost", 11790)
> oma.listNodes({svcname: "11830"})
{
  "svcname": "11830",
  "type": "sequoiadb",
  "role": "data",
  "pid": 17984,
  "groupid": 1001,
  "nodeid": 1001,
  "primary": 1,
  "isalone": 0,
  "groupname": "group2",
  "starttime": "2021-07-15-16.27.47",
  "dbpath": "/opt/sequoiadb/database/data/11830/"
}
```

[^_^]:
    Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[match]:manual/Manual/Operator/Match_Operator/Readme.md
[standalone]:manual/Manual/Sequoiadb_Command/Oma/start.md
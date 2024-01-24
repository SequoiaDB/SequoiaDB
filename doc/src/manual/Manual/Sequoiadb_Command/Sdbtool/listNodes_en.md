
##NAME##

listNodes - Enumerate node information

##SYNOPSIS##

**Sdbtool.listNodes([options],[filter],[confRootPath])**

##CATEGORY##

Sdbtool

##DESCRIPTION##

This function is used to enumerate node information.

##PARAMETERS##

| Name     | Type     | Default | Description | Required or not |
| -------- | -------- | ------- | ----------- | --------------- |
| options  | object     | display information about data nodes, coordination nodes and catalog nodes by default | display specified type node | not |
| filter   | object     | display all information by default | Filtered conditions | not |
| confRootPath | string   |  `<INSTALL_DIR>/conf` | Specify the root directory of the node's configuration file (only become effective when the mode is local) | not       |

The detail description of 'options' parameter is as follow:

| Attributes | Type | Default | Format | Description |
| ---------- | ---- | ------- | ------ | ----------- |
| type       | string | db  | { type: "all" }<br>{ type: "db" }<br>{ type: "om" }<br>{ type: "cm" } | display information about all nodes <br>display information about data nodes, coordination nodes and catalog nodes<br>display infomation about om node<br>display infomation about cm node |
| mode       | string | run | { mode: "run" }<br>{ mode: "local" } | display infomation about running nodes<br>display infomation about all nodes in the root directory of the configuration file | 
| role       | string | --- | { role: "data" }<br>{ role: "coord" }<br>{ role: "catalog" }<br>{ role: "standalone" }<br>{ role: "om" }<br>{ role: "cm" } | display infomation about data nodes<br>display infomation about coord nodes<br>display infomation about catalog nodes<br>display infomation about standalone nodes<br>display infomation about om node<br>display infomation about cm node |
| svcname    | string | --- | { svcname: "11790" } | display node information of the specified port | 
| showalone  | boolean   | false | { showalone: true }<br>{ showalone: false } | whether to diplay information about the cm node started in standalone mode |
| expand     | boolean   | false | { expand: true }<br>{ expand: false } | whether to display detailed extended configuration |

   >**Note:**
   >
   > - Cm node has a standalone startup mode. In addition to the current cm node, you can also start a cm node as a temporary cm node in standalone mode(start the cm node to specify the standalone parameter) and the cm node's default survival time is 5 minutes. 
   > - When specifying multiple svcnames, you can separate the svcnames with ",".
   > - The optional parameter filterObj supports the AND, the OR, the NOT and exact matching of some fields in the result, and the result set is filtered.

##RETURN VALUE##

When the function executes successfully, it will return an object of type BSONArray. Users can get a list of node details through this object. Field descriptions are as follows:

| Name | Type | Description |
|--------|------|------|
| svcname | string | Node port number |
| type | string | Node type |
| role | string | Node role，data represents data node, coord represents coord node and catalog represents catalog node |
| pid | int32 | Process ID |
| groupid | int32 | Node's replication group ID |
| nodeid | int32 | Node ID |
| primary | int32 | Whether the node is the primary node. 1 is the primary node and 0 is the standby node |
| isalone | int32 | Whether the node starts in standalone mode (only become effective when the node type is cm) |
| groupname | string | Node's replication group id |
| starttime | string | Startup time of the node |
| dbpath | string | The path where the node stores the data file |

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens，use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the error code. For more details, refer to [Troubleshooting][error_guide].

##VERSION##

v3.2 and above

##EXAMPLES##

* Display node information.

    ```lang-javascript
    > Sdbtool.listNodes( { type: "all", mode: "local", role: "data", svcname: "20000, 40000" } )
    {
      "svcname": "20000",
      "type": "sequoiadb",
      "role": "data",
      "pid": 17390,
      "groupid": 1000,
      "nodeid": 1000,
      "primary": 1,
      "isalone": 0,
      "groupname": "db1",
      "starttime": "2019-05-31-17.14.14",
      "dbpath": "/opt/trunk/database/20000/"
    }
    {
      "svcname": "40000",
      "type": "sequoiadb",
      "role": "data",
      "pid": 17399,
      "groupid": 1001,
      "nodeid": 1001,
      "primary": 0,
      "isalone": 0,
      "groupname": "db2",
      "starttime": "2019-05-31-17.14.14",
      "dbpath": "/opt/trunk/database/40000/"
    }
    ```

* Display node information and filter the result set.

    ```lang-javascript
    > Sdbtool.listNodes( { type: "all", mode: "local", role: "data", svcname: "20000, 40000" }, { groupname: "db2" } )
    {
      "svcname": "40000",
      "type": "sequoiadb",
      "role": "data",
      "pid": 17399,
      "groupid": 1001,
      "nodeid": 1001,
      "primary": 0,
      "isalone": 0,
      "groupname": "db2",
      "starttime": "2019-05-31-17.14.14",
      "dbpath": "/opt/trunk/database/40000/"
    }
    ```

* Display node information and specify root path of the system configuration file.

    ```lang-javascript
    > Sdbtool.listNodes( { mode: "local", svcname: "40000" }, { groupname: "db2" }, "/opt/sequoiadb/conf" )
    {
      "svcname": "40000",
      "type": "sequoiadb",
      "role": "data",
      "pid": 17399,
      "groupid": 1001,
      "nodeid": 1001,
      "primary": 0,
      "isalone": 0,
      "groupname": "db2",
      "starttime": "2019-05-31-17.14.14",
      "dbpath": "/opt/trunk/database/40000/"
    }
    ```


[^_^]:
     Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/faq.md
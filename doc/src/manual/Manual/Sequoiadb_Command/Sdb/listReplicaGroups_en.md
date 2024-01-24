##NAME##

listReplicaGroups - Enumerate replication group information

##SYNOPSIS##

**db.listReplicaGroups()**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to enumerate replication group information.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, it will return a detailed list of collections through the cursor.Users can refer to [$LIST_GROUP][LIST_GROUP] to get the returned field information.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens，use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the error code. For more details, refer to [Troubleshooting][error_guide].

##VERSION##

v2.0 and above

##EXAMPLES##

* Return all replication group information

    ```lang-javascript
    > db.listReplicaGroups()
    {
    "ActiveLocation": "GuangZhou",
    "Group": 
    [
      {
        "dbpath": "/opt/sequoiadb/data/11800",
        "HostName": "vmsvr2-suse-x64",
        "Service": [
          {
            "Type": 0,
            "Name": "11800"
          },
          {
            "Type": 1,
            "Name": "11801"
          },
          {
            "Type": 2,
            "Name": "11802"
          },
          {
            "Type": 3,
            "Name": "11803"
          }
        ],
        "NodeID": 1000,
        "Location": "GuangZhou"
      },
      {
        "dbpath": "/opt/sequoiadb/data/11850",
        "HostName": "vmsvr2-suse-x64",
        "Service": [
          {
            "Type": 0,
            "Name": "11850"
          },
          {
            "Type": 1,
            "Name": "11851"
          },
          {
            "Type": 2,
            "Name": "11852"
          },
          {
            "Type": 3,
            "Name": "11853"
          }
        ],
        "NodeID": 1001
      }
    ],
    "GroupID": 1001,
    "GroupName": "group",
    "Locations": [
      {
        "Location": "GuangZhou",
        "LocationID": 1,
        "PrimaryNode": 1000
      }
    ],
    "PrimaryNode": 1001,
    "Role": 0,
    "Status": 1,
    "Version": 5,
    "_id": {
      "$oid": "517b2fc33d7e6f820fc0eb57"
      }
    }
    ```

    This replication group has two nodes: 11800 and 11850, of which 11850 is the master node. For replication Group details,refer to [Copy List][LIST_GROUPS].


[^_^]:
     Links
[LIST_GROUP]:manual/Manual/SQL_Grammar/Monitoring/LIST_GROUP.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md
[LIST_GROUPS]:manual/Manual/List/SDB_LIST_GROUPS.md
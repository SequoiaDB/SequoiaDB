##NAME##

getCoordRG - get a reference of coordination replication group

##SYNOPSIS##

**db.getCoordRG()**

##CATEGORY##

Sdb

##DESCRIPTION##

The function is used to obtain the reference of the coordination replication group, and user can perform related operation on the replication group through the reference.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, returns an object of type SdbReplicaGroup.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens，use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

Get a reference of coordination replication group.

```lang-javascript
> var rg = db.getCoordRG()
```

Get the details of the replication group through the reference.

```lang-javascript
> rg.getDetail()
{
  "Group": [
    {
      "HostName": "sdbserver",
      "Status": 1,
      "dbpath": "/opt/sequoiadb/database/coord/11810/",
      "Service": [
        {
          "Type": 0,
          "Name": "11810"
        },
        {
          "Type": 1,
          "Name": "11811"
        },
        {
          "Type": 2,
          "Name": "11812"
        }
      ],
      "NodeID": 2
    }
  ],
  "GroupID": 2,
  "GroupName": "SYSCoord",
  "Role": 1,
  "SecretID": 112493285,
  "Status": 1,
  "Version": 2,
  "_id": {
    "$oid": "5ff52af65c0657cec2f706d9"
  }
}
```

[^_^]:
    links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
##NAME##

getDetailObj - get detailed information of the current replica group

##SYNOPSIS##

**rg.getDetailObj()**

##CATEGORY##

Replica Group

##DESCRIPTION##

Get detailed information of the current replica group, such as group id, group status, node information in the group, etc.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, it will return the current group details of type BSONObj. 

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get the error message or use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the error code. For more details, refer to [Troubleshooting](troubleshooting/general/general_guide.md).

##VERSION##

v3.2.7 and above

v3.4.2 and above

##EXAMPLES##

Get details of a replication group named "group1", which exists with one node.

```lang-javascript
> var rg = db.getRG("group1") 
> rg.getDetailObj()
```

The result is as follow:

```lang-text
{
  "ActiveLocation": "GuangZhou",
  "Group": [
    {
      "HostName": "localhost",
      "Status": 1,
      "dbpath": "/opt/sequoiadb/database/data/11830/",
      "Service": [
        {
          "Type": 0,
          "Name": "11830"
        },
        {
          "Type": 1,
          "Name": "11831"
        },
        {
          "Type": 2,
          "Name": "11832"
        }
      ],
      "NodeID": 1002,
      "Location": "GuangZhou"
    }
  ],
  "GroupID": 1001,
  "GroupName": "group1",
  "Locations": [
    {
      "Location": "GuangZhou",
      "LocationID": 1,
      "PrimaryNode": 1002
    }
  ],
  "PrimaryNode": 1004,
  "Role": 0,
  "Status": 1,
  "Version": 7,
  "_id": {
    "$oid": "580043577e70618777a2cf39"
  }
}
```
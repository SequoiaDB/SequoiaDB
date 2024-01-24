
##NAME##

getDetailObj - Get the detail of current node

##SYNOPSIS##
***node.getDetailObj()***

##CATEGORY##

SdbNode

##DESCRIPTION##

Get the detail of current node. 

##PARAMETERS##

none

##RETURN VALUE##

When the function executes successfully, it will return the current
node details of type BSONObj.

When the function fails, an exception will be thrown and an error
message will be printed.

##ERRORS##

When exception happens, use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md) and use [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) to get error message. For more details, refer to [Troubleshooting](manual/FAQ/faq_sdb.md).

##VERSION##

v3.2.8 and above, v3.4.2 and above, v5.0.2 and above

##EXAMPLES##

* Retrun the detail of current node

```lang-javascript
> node.getDetailObj()
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
  "Location": "GuangZhou",
  "NodeID": 1002,
  "GroupID": 1001,
  "GroupName": "group1"
}
```

##NAME##

listGroups - get all replication groups in the specified domain

##SYNOPSIS##

**domain.listGroups()**

##CATEGORY##

SdbDomain

##DESCRIPTION##

The function is used to get all replication groups in the specified domain.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, it will return the replication group information contained in the specified domain through the cursor.

When the function fails, an exception will be thrown and an error message will be printed.


##ERRORS##

When the exception happens，use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.0 and above

##EXAMPLES##

Get the replication group information contained in the specified domain. 

```lang-javascript
> var domain = db.getDomain("mydomain")
> domain.listGroups()
{
  "_id": {
    "$oid": "5b92291ec5e807b5e32582cc"
  },
  "Name": "mydomain",
  "Groups": [
    {
      "GroupName": "db1",
      "GroupID": 1000
    },
    {
      "GroupName": "db2",
      "GroupID": 1001
    }
  ],
  "AutoSplit": true
}
```


[^_^]:
    links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[faq]:manual/FAQ/faq_sdb.md
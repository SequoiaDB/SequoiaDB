##NAME##

getCataRG - get a reference of catalog replication group.

##SYNOPSIS##

**db.getCataRG()**

##CATEGORY##

Sdb

##DESCRIPTION##

The function is used to obtain the reference of the catalog replication group, and user can perform related operation on the replication group through the reference.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, retuens an object of type SdbReplicaGroup.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens，use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##EXAMPLES##

Get a reference of catalog replication group.

```lang-javascript
> var rg = db.getCataRG()
```

Get the details of the replication group through reference.

```lang-javascript
> rg.getDetail()
{
  "Group": [
    {
      "dbpath": "/opt/sequoiadb/database/catalog/11800",
      "HostName": "u1604-lxy",
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
      "NodeID": 1,
      "Status": 1
    }
  ],
  "GroupID": 1,
  "GroupName": "SYSCatalogGroup",
  "PrimaryNode": 1,
  "Role": 2,
  "SecretID": 1519705199,
  "Status": 1,
  "Version": 1,
  "_id": {
    "$oid": "5ff52af05c0657cec2f706d5"
  }
}
```

[^_^]:
     links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[Sequoiadb_error_code]:manual/Manual/Sequoiadb_error_code.md
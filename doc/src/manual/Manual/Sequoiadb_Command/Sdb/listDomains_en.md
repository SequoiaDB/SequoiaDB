##NAME##

listDomains - list all created domains

##SYNOPSIS##

**db.listDomains([cond], [sel], [sort])**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to list all user-created domains in the system. 

##PARAMETERS##

| Name    | Type   | Description    | Required or Not |
|---------|--------|----------------|-----------------|
| cond    | object   | Match condictions, and only return records that match cond. When null, return all.              | Not             |
| sel     | object   | Select the returned field name. When null, return all field names.                        | Not             |
| sort    | object   | Sort the returned records by the selected field. 1 is ascending and -1 is descending.        | Not             |

##RETURN VALUE##

When the function executes successfully, it will return an object of type SdbCursor. Users can get the detailed information list of the domain through this object. For field descriptions, refer to [SYSDOMAINS colletion][SYSDOMAINS].

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens，use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the error code. For more details, refer to [Troubleshooting][error_guide].

##VERSION##

v2.0 and above

##EXAMPLES##

List all user-created domains in the system. 

```lang-javascript
> db.listDomains()
{
  "_id": {
	"$oid": "5811641e3426f0835eef45bf"
  },
  "Name": "mydomain",
  "Groups": [
	{
	  "GroupName": "group1",
	  "GroupID": 1001
	},
	{
	  "GroupName": "group2",
	  "GroupID": 1002
	},
	{
	  "GroupName": "group3",
	  "GroupID": 1000
	}
  ]
}
```

[^_^]:
     links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md
[SYSDOMAINS]:manual/Manual/Catalog_Table/SYSDOMAINS.md
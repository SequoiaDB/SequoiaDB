##NAME##

listCollectionSpaces - Enumerate collection space information

##SYNOPSIS##

**db.listCollectionSpaces()**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to enumerate all the collection space information in the database.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully,it will return a detailed list of collections through the cursor.Users can refer to [$LIST_CS][LIST_CS] to get the returned field information.

When the function fails,an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens，use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the error code. For more details, refer to [Troubleshooting][error_guide].

##VERSION##

v2.0 and above

##EXAMPLES##

* List all collection space information in the database

	```lang-javascript
	> db.listCollectionSpaces()
	{
	  "Name":"sample"
	}
	```

[^_^]:
     Links
[LIST_CS]:manual/Manual/SQL_Grammar/Monitoring/LIST_CS.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md
##NAME##

listTasks - Enumerate background tasks

##SYNOPSIS##

**db.listTasks( [cond], [sel], [sort], [hint] )**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to enumerate all background tasks in the database.

##PARAMETERS##

| Name    | Type   | Description    | Required or Not |
|---------|--------|----------------|-----------------|
| cond   | Json  | Filter conditions of tasks | Not  |
| sel 	 | Json  | Selection field of tasks | Not 		|
| sort   | Json   | Sort the returned records by the selected field. 1 is ascending and -1 is descending.        	| Not 	    |
| hint 	 | Json  | Reserved item 														| Not 	    |

##RETURN VALUE##

When the function executes successfully, it will return a detailed list of collections through the cursor.Users can refer to [SYSTASKS collection][SYSTASKS] to get the returned field information.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens，use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the error code. For more details, refer to [Troubleshooting][error_guide].

##VERSION##

v2.0 and above

##EXAMPLES##

* List all background tasks of the system

	```lang-javascript
	> db.listTasks()
	```

[^_^]:
     Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md
[SYSTASKS]:manual/Manual/Catalog_Table/SYSTASKS.md
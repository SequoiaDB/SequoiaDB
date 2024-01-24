##NAME##

listProcedures - Enumerate stored procedures

##SYNOPSIS##

**db.listProcedures( [cond] )**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to enumerate all stored procedure functions.

##PARAMETERS##

| Name    | Type   | Description    | Required or Not |
|---------|--------|----------------|-----------------|
| cond 	 | Json | Condition is empty, enumerate all the functions, not empty, the enumeration function in line with the conditions. | Not	  |


##RETURN VALUE##

When the function executes successfully, it will return a detailed list of collections through the cursor.Users can refer to [STOREPROCEDURES collection][STOREPROCEDURES] to get the returned field information.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens，use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the error code. For more details, refer to [Troubleshooting][error_guide].

##VERSION##

v2.0 and above

##EXAMPLES##

* List all function information

	```lang-javascript
	> db.listProcedures()
	{ "_id" : { "$oid" : "52480389f5ce8d5817c4c353" }, 
	  "name" : "sum", 
	  "func" : "function sum(x, y) {return x + y;}", 
	  "funcType" : 0 
	}
	{ "_id" : { "$oid" : "52480d3ef5ce8d5817c4c354" }, 
	  "name" : "getAll", 
	  "func" : "function getAll() {return db.sample.employee.find();}", 
	  "funcType" : 0 
	}
	```

* Specifies the record that returns sum as the funtion name

	```lang-javascript
	> db.listProcedures({name:"sum"})
	{ "_id" : { "$oid" : "52480389f5ce8d5817c4c353" }, 
	  "name" : "sum", 
	  "func" : "function sum(x, y) {return x + y;}", 
	  "funcType" : 0 
	}
	```

[^_^]:
     Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md
[STOREPROCEDURES]:manual/Manual/Catalog_Table/STOREPROCEDURES.md
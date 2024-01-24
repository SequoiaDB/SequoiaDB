##NAME##

transBegin - begin the transaction

##SYNOPSIS##

**db.transBegin()**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to begin the transaction. SequoiaDB database transaction refers to a series of operations performed as a single logical unit of work. Transaction processing can ensure that unless all operations within the transactional unit are successfully completed, data-oriented resources will not be permanently updated.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

* Begin the transaction.

	```lang-javascript
	> db.transBegin()
	```

* Insert a record.

	```lang-javascript
	> cl.insert({date: 99, id: 8, a: 0})
	```

* Roll back the transaction, the inserted record will be rolled back and there is no record in the collection.

	```lang-javascript
	> db.transRollback()
	> cl.count()
	```

* Begin the transaction.

	```lang-javascript
	> db.transBegin()
	```

* Insert a record.

	```lang-javascript
	> cl.insert({date: 99, id: 8, a: 0})
	```

* Commit the transaction and the inserted record will be written to the database.

	```lang-javascript
	> db.transCommit()
	> cl.count()
	1
	```


[^_^]:
   links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
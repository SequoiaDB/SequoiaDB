
##NAME##

dropAutoIncrement - Drop one or more auto increment fields in a specified collection.

##SYNOPSIS##

**db.collectionspace.collection.dropAutoIncrement(<name|names>)**

##CATEGORY##

Collection

##DESCRIPTION##
Drop one or more auto increment fields in a specified collection.

##PARAMETERS##

* `name|names` ( *String | Array of Strings*, *Required* )

	Auto increment field name.

##RETURN VALUE##

On success, return void.

On error, exception will be thrown and ouput the error message.

##EXAMPLES##

1. Drop an auto increment field.

	```lang-javascript
	> db.sample.employee.dropAutoIncrement( "studentID" )
	```
   
2. Drop multiple auto increment fields.

	```lang-javascript
	> db.sample.employee.dropAutoIncrement( [ "comID", "innerID" ] )
	```



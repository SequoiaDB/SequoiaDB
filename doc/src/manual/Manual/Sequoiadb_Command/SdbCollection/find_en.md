##NAME##

find - select records from database

##SYNOPSIS##

**db.collectionspace.collection.find([cond], [sel])**

**db.collectionspace.collection.find([cond], [sel]).hint([hint])**

**db.collectionspace.collection.find([cond], [sel]).hint([hint]).flags(\<flags\>)**

**db.collectionspace.collection.find([cond], [sel]).skip([skipNum]).limit([retNum]).sort([sort])**

**db.collectionspace.collection.find([cond], [sel])[.hint([hint])][.skip([skipNum])][.limit([retNum])][.sort([sort])]**

**db.collectionspace.collection.find([SdbQueryOption])**

##CATEGORY##

Collection

##DESCRIPTION##

This function is used to query the records that meet the conditions.

##PARAMETERS##

- cond ( *object, optional* )

	Selecting condtion. If it is null, it will find all the records. 
    If it is not null, it will find records that matches the condition.

- sel ( *object, optional* )

	It chooses fields to be returned. If it is null, it will return all the fields.
    If a field doesn't exist, it will return the same contents which was input. 
	
	Format: {"filed1":"", "filed2":"", "filed3":""}

- hint ( *object, optional* )

	specified the hint for query.
	* when not specified 'hint', it is up to the database to decide whether 
      to use the index and which index to be used. 
	* when 'hint' is {"":null}, table scan.
	* when 'hint' contains only one index, such as: {"":"myIdx"}, the query will be
      made using the index named "myIdx" in the currrent collection; However, when index "myIdx" does not exist in current collection, query goes with table scan.
	* when 'hint' contains several indexes, such as: {"1":"idx1","2":"idx2","3":"idx3"},
		 	  the query	will be made using one of the three indexes described above. Which 
	  index is used eventually, determined by the database evaluation.

- flags ( *object, optional* )

    Specify the flag bit to traverse the result set. For specific usage user can refer to [flags()][flags].

- skipNum ( *number, optional* )

	specified where returns from the record of the result set. The default value is 0, 
	means returns from the first record of the result set.

- retNum ( *number, optional* )

	specified how many records to be return from the result set. The default value is -1,
	means returns all the recoreds since the position of "skipNum".

- sort ( *object, optional* )

	Specifies whether the result set is sorted by the specified field name. The 
  	value of the field name is 1 or -1, such as: {"name":1,"age":-1}.
	* when not specified 'sort', means the result set is not sorted.
	* when the value of field name is 1, means sort by field name in ascending order.
	* when the value of field name is -1, means sort by field name in decending order.

- SdbQueryOption ( *object, optional* )

	Use an object to specify record query parameters.For more detial, please  reference to [SdbQueryOption](manual/Manual/Sequoiadb_command/AuxiliaryObjects/SdbQueryOption.md).

>**Note：**
>
> * The parameter 'sel' is an object, the values of it's fields are empty string, 
  database only care about the name of the fields.
> * The parameter 'hint' is an object, the name of it's fields can be any unique string,
  database onlu care about the value of the fileds.

##RETURN VALUE##

When the function executes successfully, it will return an object of type SdbQuery.Users can get the result collection of the query.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `find()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -2 | SDB_OOM | No memory available| Check the settings and usage of physical memory and virtual memory.|
| -6 | SDB_INVALIDARG | Parameter error | Check whether the parameters are filled in correctly.|
| -34 | SDB_DMS_CS_NOTEXIST | Collection space does not exist| Check whether the collection space exists.|
| -23 | SDB_DMS_NOTEXIST| Collection does not exist | Check whether the collection exists.|

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##HISTORY##

v1.0 and above

##EXAMPLES##

- query all the records without specified condition and selector.

	```lang-javascript
	> db.sample.employee.find()
	```

- query records with the specified condition. Return the records which name
   is "Tom" and "age" is greater then 25.

	```lang-javascript
	db.sample.employee.find( { age: { $gt: 25 }, name: "Tom" } )
	```
- specified the fields to be returned. when we have recored { age: 25, type: "system" }
   and { age: 20, name: "Tom", type: "normal" }, the follow operation return the contents 
   of field "age" and field "name".

	```lang-javascript
	db.sample.employee.find( null, { age: "", name: "" } )
	 	{
	    	"age": 25,
	      	"name": ""
	 	}
	 	{
	      	"age": 20,
	      	"name": "Tom"
	 	}
	```
- specifed which index to be use to query.

	```lang-javascript
   > db.sample.test.find( {age: {$exists:1} } ).hint( { "": "ageIndex" } )
	{
    		"_id": {
    		"$oid": "5812feb6c842af52b6000007"
    		},
    		"age": 10
	}
	{
    		"_id": {
    		"$oid": "5812feb6c842af52b6000008"
    		},
    		"age": 20
	}
	```

- select records from collection 'sample.employee' and return records which field "age" is greater 
   than 10 by using [$gt](manual/Manual/Operator/match_operator/gt.md). when get the result set,
   we skip the first 3 records and return 5 records.

	```lang-javascript
	> db.sample.employee.find( { age: { $gt: 10 } } ).skip(3).limit(5)
	```
	when the number of records in the result set is not greater that 3, no record is returned.

	when the number of records in the result set is greater that 3, returns up to 5 records.

- select records from collection 'sample.employee' and return records which filed 'age' is greater
   than 20. we only want filed 'age' and field 'name' to be return. The returned records are
   sort by field 'age' in ascending order.

	```lang-javascript
	db.sample.employee.find( { age: { $gt: 20 } }, { age: "", name: "" } ).sort( { age: 1 }
	```

 'sort' can only apply to the returned fields.

- specified a useless field name to sort.

	```lang-javascript
	db.sample.employee.find( { age: { $gt: 20 } }, { age: "", name: "" } ).sort( { "sex": 1 } )
	```

 for the return records do not contain field "sex", so the sort request does not work.

- Find records which contain "rock climbing" in collection "employee" by using full text search

 ```lang-javascript
> db.sample.employee.find({"":{"$Text":{"query":{"match":{"about" : "rock climbing"}}}}})
 ```

[^_^]:
    Links
[overview]:manual/Manual/Operator/Match_Operator/Readme.md
[text_index]:manual/Distributed_Engine/Architecture/Data_Model/text_index.md
[QueryOption]:manual/Manual/Sequoiadb_Command/AuxiliaryObjects/SdbQueryOption.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[faq]:manual/FAQ/faq_sdb.md
[gt]:manual/Manual/Operator/Match_Operator/gt.md
[flags]:manual/Manual/Sequoiadb_Command/SdbQuery/flags.md
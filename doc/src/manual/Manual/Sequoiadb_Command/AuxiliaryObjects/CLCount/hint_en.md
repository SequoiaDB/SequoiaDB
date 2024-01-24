##NAME##

hint - Traverse the result set by the specified index.

##SYNOPSIS##

***CLCount.hint(\<hint\>)***

##CATEGORY##

CLCount

##DESCRIPTION##

Traverse the result set by the specified index.

##PARAMETERS##

| Name | Type     | Default | Description         | Required or not |
| ---- | -------- | ------- | ------------------- | --------------- |
| hint | JSON     | ---     | specify query index | yes             |

> **Note:** 
 
> 1. The parameter "hint" is a Json object. The database does not care about the field of the object, but instead uses its field value to confirm the name of the index to use. When the field value is null, it indicates a table scan. The format of the parameter "hint" are as follow: ```{ "": null }, { "": "indexname" }, { "0": "indexname0", "1": "indexname1", "2": "indexname2" }```.

> 2. Before SequoiaDB-v3.0, When an index is specified by using hint(), once the database traverses to an index that can be used (or a table scan), it stops traversing and moves to the index (or table scan) for data lookup. 

> 3. After  SequoiaDB-v3.0, When the database selects an index, it performs a comprehensive analysis based on the statistical model of the data and the index, and finally selects the most appropriate index to use. So, starting with v3.0, when you specify multiple indexes using hint(), the database will be able to select the index that best fits the current query.

##RETURN VALUE##

On success, returns the cursor of the query result set.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

* Specify the index to count the total number of records in the current collection.

```lang-javascript
> var db = new Sdb( "localhost", 11810 )
> db.foo.bar.find().count().hint( { "": "ageIndex" } )
50004
```

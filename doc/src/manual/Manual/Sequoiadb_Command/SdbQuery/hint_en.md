
##NAME##

hint - Traversing the result set by the specified index.

##SYNOPSIS##

***query.hint( \<hint\> )***

##CATEGORY##

SdbQuery

##DESCRIPTION##

Traversing the result set by the specified index.

##PARAMETERS##

| Name | Type     | Default | Description                              | Required or not |
| ---- | -------- | ------- | ---------------------------------------- | --------------- |
| hint | JSON     | ---     | specify access plans to speed up queries | yes             |

> **Note:** 

> 1. The parameter "hint" is a Json object. The database does not care about the field of the object, but instead uses its field value to confirm the name of the index to use. When the field value is null, it indicates a table scan. The format of the parameter "hint" are as follow: ```{ "": null }, { "": "indexname" }, { "0": "indexname0", "1": "indexname1", "2": "indexname2" }```.

> 2. Before SequoiaDB-v3.0, When an index is specified by using hint(), once the database traverses to an index that can be used (or a table scan), it stops traversing and moves to the index (or table scan) for data lookup. 

> 3. After  SequoiaDB-v3.0, When the database selects an index, it performs a comprehensive analysis based on the statistical model of the data and the index, and finally selects the most appropriate index to use. So, starting with v3.0, when you specify multiple indexes using hint(), the database will be able to select the index that best fits the current query.

##RETURN VALUE##

On success, returns the cursor of the query result set.

On error, exception will be thrown.

##ERRORS##

When exception happens, use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md) and use [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) to get error message. For more details, refer to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* It is mandatory to query the table scan.

```lang-javascript
> db.sample.employee.find( {age: 100 } ).hint( { "": null } )
```

* Use the index ageIndex to iterate through the records of the age field in the collection employee and return.

```lang-javascript
> db.sample.employee.find( {age: {$exists:1} } ).hint( { "": "ageIndex" } )
{
  "_id": {
    "$oid": "5cf8aef75e72aea111e82b38"
  },
  "name": "tom",
  "age": 20
}
{
  "_id": {
    "$oid": "5cf8aefe5e72aea111e82b39"
  },
  "name": "ben",
  "age": 21
}
{
  "_id": {
    "$oid": "5cf8af065e72aea111e82b3a"
  },
  "name": "alice",
  "age": 19
}
```

* Provide several indexes for database selection. The database will select the optimal index to use based on the data and index statistics.

```lang-javascript
> db.sample.employee.find( {age: 10 } ).hint( { "1": "aIndex", "2": "bIndex", "3":"cIndex" } )
```
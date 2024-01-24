
##NAME##

flags - Specify flag bit to traverse the result set.

##SYNOPSIS##

***query.flags( \<flag\> )***

##CATEGORY##

SdbQuery

##DESCRIPTION##

Specify flag bit to traverse the result set.

##PARAMETERS##

| Name | Type | Default | Description | Required or not |
| ---- | ---- | ------- | ----------- | --------------- |
| flag | list | ---     | flag bit    | yes             |

The optional values of the 'flag' parameter are as follows：

| Optional values | Description                                                       |
| --------------- | ----------------------------------------------------------------- |
| SDB_FLG_QUERY_FORCE_HINT | When add this flag, it will force to use specified hint to query, if database don't have the specified index, fail to query and return an error |
| SDB_FLG_QUERY_PARALLED | When add this flag, it will enable parallel sub query, each sub query will finish scanning different part of the data |
| SDB_FLG_QUERY_WITH_RETURNDATA | In general, query won't return data but a cursor. And the cursor is used to get data. When add this flag, it will return data in query response. This flag is enabled by default |
| SDB_FLG_QUERY_PREPARE_MORE | During the query, the server will perform multiple transmissions with the client to return the query result to client. When add this flag, the server will transmit more data to the client each time. It will reduce the number of transmissions between the server and the client and reduce network overhead. |
| SDB_FLG_QUERY_FOR_UPDATE | Acquire U lock on the records that are read. When the session is in transaction and setting this flag, the transaction lock will not released until the transaction is committed or rollback. When the session is not in transaction, the flag does not work. |
| SDB_FLG_QUERY_FOR_SHARE | Acquire S lock on the records that are read. When the session is in transaction and setting this flag, the transaction lock will not released until the transaction is committed or rollback. When the session is not in transaction, the flag does not work. |

>**Note:**

>Suppose the user specifies an index that is not in the collection to query data, the database will perform a full table query instead of an index query. However, if the user specifies an index that is not in the collection and specifies SDB_FLG_QUERY_FORCE_HINT flag to query data, it will fail too query and return an error.

##RETURN VALUE##

On success, returns the cursor of the query result set.

On error, exception will be thrown.

##ERRORS##

When exception happens, use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md) and use [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) to get error message. For more details, refer to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Force to use specified hint to query.

```lang-javascript
> db.sample.employee.find().hint( { "": "ageIndex" } ).flags( SDB_FLG_QUERY_FORCE_HINT )
{
   "_id": {
     "$oid": "5d412cfa614afb5557b2b41d"
   },
   "name": "fang",
   "age": 18
}
{
   "_id": {
     "$oid": "5d412cfa614afb5557b2b41c"
   },
   "name": "alice",
   "age": 19
}
{
   "_id": {
     "$oid": "5d412cfa614afb5557b2b41b"
   },
   "name": "ben",
   "age": 21
}

> db.sample.employee.find().hint( { "": "notExistIndex" } ).flags( SDB_FLG_QUERY_FORCE_HINT )
uncaught exception: -53
Invalid hint
Takes 0.003438s.
```

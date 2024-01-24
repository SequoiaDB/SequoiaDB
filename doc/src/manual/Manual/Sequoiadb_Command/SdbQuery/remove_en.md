
##NAME##

remove - Delete the result set after the query.

##SYNOPSIS##

***query.remove()***

##CATEGORY##

SdbQuery

##DESCRIPTION##

Delete the result set after the query.

>**Note:**  

>1. Remove() cannot be used with count() and update().  

>2. If remove() is used with sort(), it must use an index when sorts on a single node.  

>3. When remove is used with limit() and skip() in a cluster, it must ensure that the query conditions are executed on a single node or on a single child table.

##PARAMETERS##

NULL

##RETURN VALUE##

On success, return the cursor of the deleted result set. 

On error, exception will be thrown.

##ERRORS##

When exception happens, use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md) and use [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) to get error message. For more details, refer to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Select the record that the age field value greater than (with using [$gt](reference/operator/match_operator/gt.md)) 10 under the collection, employee, and remove the records.

```lang-javascript
> db.sample.employee.find( { age: { $gt: 10 } } ).remove()
{
  "_id": {
    "$oid": "5d2c4455f6d7aeedc15ddf87"
  },
  "name": "tom",
  "age": 18
}

```
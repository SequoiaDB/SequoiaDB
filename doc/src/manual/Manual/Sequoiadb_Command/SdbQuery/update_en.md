
##NAME##

update - Update the result set after the query.

##SYNOPSIS##

***query.update( \<rule\>, [returnNew], [options] )***

##CATEGORY##

SdbQuery

##DESCRIPTION##

Update the result set after the query.

> **Note:** 

> 1. update() cannot be used with count() and remove().

> 2. If update() is used with sort(), it must use an index when sorts on a single node.

> 3. When update() is used with limit() and skip() in a cluster, it must ensure that the query conditions are executed on a single node or on a single child table.

##PARAMETERS##

| Name      | Type | Default | Description | Required or not |
| --------- | ---- | ------- | ----------- | --------------- |
| rule      | JSON | ---     | update rules and records are updated according to specified rules | yes |
| returnNew | bool | false   | whether to return the record after the update | not |
| options   | JSON | ---     | specify partition key properties | not |

The detail description of 'options' parameter is as follow:

| Attributes      | Type | Default | Description | Required or not |
| --------------- | ---- | ------- | ----------- | --------------- |
| KeepShardingKey | bool | false   | whether to retain the partition key field | not |

##RETURN VALUE##

On success, return result set.

On error, exception will be thrown.

##ERRORS##

When exception happens, use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md) and use [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) to get error message. For more details, refer to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Select the record that the age field value greater than (with using [$gt](reference/operator/match_operator/gt.md)) 10 under the collection, employee, and add (with using [$inc](reference/operator/update_operator/inc.md)) one to the age field.

```lang-javascript
> db.sample.employee.find( { age: { $gt: 10 } } ).update( { $inc: { age: 1 } }, true )
{
  "_id": {
    "$oid": "5d006c45e846796ae69f85a9"
  },
  "age": 21,
  "name": "tom"
}
{
  "_id": {
    "$oid": "5d006c45e846796ae69f85aa"
  },
  "age": 22,
  "name": "ben"
}
{
  "_id": {
    "$oid": "5d006c45e846796ae69f85ab"
  },
  "age": 23,
  "name": "alice"
}
```

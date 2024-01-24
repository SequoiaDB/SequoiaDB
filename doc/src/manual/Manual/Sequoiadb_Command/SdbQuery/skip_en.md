
##NAME##

skip - Specify which record the result set will return from.

##SYNOPSIS##

***query.skip( \<num\> )***

##CATEGORY##

SdbQuery

##DESCRIPTION##

Specify which record the result set will return from.

##PARAMETERS##

| Name | Type | Default | Description | Required or not |
| ---- | ---- | ------- | ----------- | --------------- |
| num  | int  | ---     | the position at which the result set begins to return | yes |

>**Note:**

>If the number of records in the result set is less than ( num + 1 ), it will return null.

##RETURN VALUE##

On success, return result set.

On error, exception will be thrown.

##ERRORS##

When exception happens, use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md) and use [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) to get error message. For more details, refer to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Return from the second record.

```lang-javascript
> db.sample.employee.find().skip(1)
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

* Return from the forth record. (The current collection  has only three recoeds)

```lang-javascript
> db.sample.employee.find().skip(3)
Return 0 row(s).
```



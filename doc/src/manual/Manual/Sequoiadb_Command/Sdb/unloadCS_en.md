##NAME##

unloadCS -  Unload the specific collection space from memory.

##SYNOPSIS##

***db.unloadCS( \<csName\>, [options] )***

##CATEGORY##

Sdb

##DESCRIPTION##

Unload the specific collection space from memory.

##PARAMETERS##

| Name    | Type   | Default | Description                          | Required or not |
| ------- | ------ | ------- | ------------------------------------ | --------------- |
| csName  | string | ---     | collection space name                | yes             |
| options | JSON   | NULL    | [command position parameter](manual/Manual/Sequoiadb_Command/Overview/location.md) | not             |

##RETURN VALUE##

On success, return void.

On error, exception will be thrown.

##ERRORS##

When exception happens, use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md) and use [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) to get error message. For more details, refer to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Query data. ( Suppose the specific collection space named "sample" exists )

```lang-javascript
> db.sample.employee.find()
{
   "_id": {
     "$oid": "5d36c9d5c6b1cee56abefc7e"
   },
   "name": "fang",
   "age": 18
}
```

* Unload the collection space named sample from memory.

```lang-javascript
> db.unloadCS( "sample" )
```

* Query data.

```lang-javascript
> db.sample.employee.find()
uncaught exception: -34
Collection space does not exist:
Collection space[sample] has been unloaded
```

* Load the collection space named into memory.

```lang-javascript
> db.loadCS( "sample" )
```

* Query data again.

```lang-javascript
> db.sample.employee.find()
{
   "_id": {
     "$oid": "5d36c9d5c6b1cee56abefc7e"
   },
   "name": "fang",
   "age": 18
}
```


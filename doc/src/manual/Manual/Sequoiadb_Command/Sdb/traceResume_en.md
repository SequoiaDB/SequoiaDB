
##NAME##

traceResume - Resume the breakpoint tracking program.

##SYNOPSIS##

***db.traceResume()***

##CATEGORY##

Sdb

##DESCRIPTION##

Turn on the database engine program tracking while db.traceOn() specifies a breakpoint. When the tracked module was blocked because of the breakpoint, db.traceResume() can wake up the module which was tracked and blocked. 

##PARAMETERS##

None

##RETURN VALUE##

On success, return void.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Connect to the data node 20000, turn on database engine program tracking

```lang-javascript
> var data = new Sdb( "localhost", 20000 )
> data.traceOn( 1000, "dms", "_dmsStorageUnit::insertRecord" )
```

* Connect to the coord node 50000. collection named bar and collectionspace named sample are belong to the data node 20000, and execute an insert operation. However, the insert operation will be blocked.
  
```lang-javascript
> var db = new Sdb( "localhost", 50000 )
> db.sample.bar.insert( { _id: 1, name: "a" } )
```

* After resume the breakpoint tracking program, the insert operation succeeded and return result we expected.

```lang-javascript
> data.traceResume()
```

* Using [traceStatus()](manual/Manual/Sequoiadb_command/Sdb/traceStatus.md) to view the tracking status of the current program. 

```lang-javascript
> data.traceStatus()
```

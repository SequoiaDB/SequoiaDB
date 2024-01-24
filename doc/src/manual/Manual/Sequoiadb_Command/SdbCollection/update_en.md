##NAME##

update - update the collection records

##SYNOPSIS##

**db.collectionspace.collection.update\(\<rule\>, \[cond\], \[hint\], \[options\]\)**

##CATEGORY##

SdbCollection

##DESCRIPTION##

This function is used to update the collection records.

##PARAMETERS##

| Name | Type| Description | Required or not |
| ------ | -------- | ---- | -------- |
| rule   | object| Update rules. The record is updated according to the content of the rule. | required |
| cond   | object| Selection condition. When it is empty, update all records. When it is not empty, update the records that meet the conditions. | not |
| hint   | object| Specify an access plan. | not |
| options| object| Options. For more details, refer to the description of options.| not |

options:

| Name          | Type | Description   | Default |
| --------------- | -------- | ------------------- | ------ |
| KeepShardingKey | boolean     | false: do not keep the partition key field in the update rule, only update the non-partition key field.<br>true: keep the partition key field in the update rule.| false  |
| JustOne         | boolean     | true: Only update one eligible record.<br>false: Update all eligible records.| false  |

> **Note:**
>
> * The usage of the parameter "hint" are same as the way of [find()][find].
> * The partition key updates are not supported on partition collection currently. If "KeepShardingKey" is true and there is a partition key field in the update rule, an error -178 will be reported.
> * When "JustOne" is true, it can only be executed on a single partition and a single subtable.


##RETURN VALUE##

When the function executes successfully, it will return an object of type BSONObj. Users can get the number of successfully updated records through this object.Field descriptions are as follows: 

| Name | Type | Description  |
|--------|------|------|
| UpdatedNum | int64 | The number of records successfully updated, including records that match but have no data changes. |
| ModifiedNum | int64 | The number of records successfully updated with data changes. |
| InsertedNum | int64 | Number of records successfully inserted. |

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `update()` function are as follows:
  
| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -178     |SDB_UPDATE_SHARD_KEY| Update partition key is not supported on partition collection. | The value of "KeepShardingKey" is false, partition key is not updated. |
| -347     |SDB_COORD_UPDATE_MULTI_NODES|When the parameter "JustOne" is true, update records across multiple partitions or subtables. | Modify the matching conditions or do not use the parameter "JustOne". |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

* Update all records in the collection according to the specified update rules, which is set the content of the "rule" parameter without setting the content of the "cond" and "hint" parameters. The following operation updates the "age" field under the collection "sample.employee", and uses [$inc][inc] to increase the value of the "age" field by 1.

    ```lang-javascript
    > db.sample.employee.update({$inc: {age: 1}})
    ```

* Select the records that meet the matching conditions, and update these records according to the update rules, which is set the "rule" and "cond" parameters. The following operation uses the match character [$exist][exists] to match the records in the update collection "sample.employee" that have the "age" field but not the "name" field, and use $unset to delete the "age" field of these records.

    ```lang-javascript
    > db.sample.employee.update({$unset: {age: ""}}, {age: {$exists: 1}, name: {$exists: 0}})
    ```

* Update records according to the access plan, assume that the specified index name exists in the collection.The following operation uses the index named "testIndex" to access the records whose "age" field value is greater than 20 in the collection "sample.employee", and adds 1 to the "age" field name of these records.

    ```lang-javascript
    > db.sample.employee.update({$inc: {age: 1}}, {age: {$gt: 20}}, {"": "testIndex"})
    ```

- The partition collection "sample.employee", and the partition key is {a: 1}. It contains the following records.

    ```lang-javascript
    > db.sample.employee.find()
    {
      "_id": {
        "$oid": "5c6f660ce700db6048677154"
      },
      "a": 1,
      "b": 1
    }
    Return 1 row(s).
    ```
 
- Specify the "KeepShardingKey" parameter: Do not keep the partition key field in the update rule. Only the non-partition key "b" field is updated, and the value of the partition key "a" field is not updated.
 
    ```lang-javascript
    > db.sample.employee.update({$set: {a: 9, b: 9}}, {}, {}, {KeepShardingKey: false})
    {
      "UpdatedNum": 1,
      "ModifiedNum": 0,
      "InsertedNum": 0
    }
    > db.sample.employee.find()
    {
      "_id": {
        "$oid": "5c6f660ce700db6048677154"
      },
      "a": 1,
      "b": 9
    }
    Return 1 row(s).
    ```
 
- Specify the "KeepShardingKey" parameter: keep the partition key field in the update rule. Since updating the partition key is currently not supported, an error will be reported.
 
    ```lang-javascript
    > db.sample.employee.update({$set: {a: 9}}, {}, {}, {KeepShardingKey: true})
    (nofile):0 uncaught exception: -178
    Sharding key cannot be updated
    ```


[^_^]:
    Links
[find]:manual/Manual/Sequoiadb_Command/SdbCollection/find.md
[inc]:manual/Manual/Operator/Update_Operator/inc.md
[exists]:manual/Manual/Operator/Match_Operator/exists.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md

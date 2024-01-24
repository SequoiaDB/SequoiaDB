##NAME##

upsert - update collection records

##SYNOPSIS##

**db.collectionspace.collection.upsert\(\<rule\>, \[cond\], \[hint\], \[setOnInsert\], \[options\]\)**

##CATEGORY##

SdbCollection

##DESCRIPTION##

This function is used to update collection records. The upsert method and the update method both update records. The difference is that when the cond parameter is used to match no records in the collection, update does nothing, while the upsert method does an insert operation.

##PARAMETERS##

| Name | Type| Description | Required or not |
| ------ | -------- | ---- | -------- |
| rule   | object| Update rules. The record is updated according to the content of the rule. | required |
| cond   | object| Selection condition. When it is empty, update all records. When it is not empty, update the records that meet the conditions. | required |
| hint   | object| Specify an access plan. | not |
| options| object| Options. For more details, refer to the description of options.| not |

options:

| Name          | Type | Description   | Default |
| --------------- | -------- | ------------------- | ------ |
| KeepShardingKey | boolean     | false: do not keep the partition key field in the update rule, and only update the non-partition key field.<br>true: keep the partition key field in the update rule| false  |
| JustOne         | boolean     | true: Only update one eligible record<br>false: Update all eligible records| false  |

> **Note:**
>
> * The usage of the parameter "hint" are same as the way of [find()][find].
> * When the "cond" parameter does not match a record in the collection, upsert will generate a record and insert it into the collection.The rules for generating records are: First, take out the key-value pairs corresponding to the $et and $all operators from the "cond" parameter, and generate an empty record if not. Then use the rule to update it, and finally add the key-value pair in "setOnInsert".
> * The partition key updates are not supported on partition collection currently. If "KeepShardingKey" is true and there is a partition key field in the update rule, an error -178 will be reported.
> * When "JustOne" is true, it can only be executed on a single partition and a single subtable.

##RETURN VALUE##

When the function executes successfully, it will return an object of type BSONObj. Users can get the number of successfully updated records through this object. Field descriptions are as follows: 

| Name | Type | Description  |
|--------|------|------|
| UpdatedNum | int64 | The number of records successfully updated, including records that match but have no data changes. |
| ModifiedNum | int64 | The number of records successfully updated with data changes. |
| InsertedNum | int64 | Number of records successfully inserted. |

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `upsert()` function are as follows:
  
| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -178     |SDB_UPDATE_SHARD_KEY| Update partition key is not supported on partition collection. | The value of "KeepShardingKey" is false, and the partition key is not updated. |
| -347     |SDB_COORD_UPDATE_MULTI_NODES|When the parameter "JustOne" is true, update records across multiple partitions or subtables. | Modify the matching conditions or do not use the parameter "JustOne". |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

Suppose there are two records in the collection "sample.employee".

```lang-json
{
  "_id": {
    "$oid": "516a76a1c9565daf06030000"
  },
  "age": 10,
  "name": "Tom"
}
{
  "_id": {
    "$oid": "516a76a1c9565daf06050000"
  },
  "a": 10,
  "age": 21
}
```

* Update all records in the collection according to the specified update rule, which is set the "rule" parameter, not the "cond" and "hint" parameters. The following operation is equivalent to using the update method to update all records in the collection "sample.employee", using [$inc][inc] to increase the value of the age field of the record by 1, and change the value of the "name" field to "Mike". For the record that does not have the "name" field , [$set][set] operator will insert the "name" field and its set value into the record, user can use the find method to view the update result.

    ```lang-javascript
    > db.sample.employee.upsert({$inc: {age: 1}, $set: {name: "Mike"}})
    {
      "UpdatedNum": 2,
      "ModifiedNum": 2,
      "InsertedNum": 0
    }
    >
    > db.sample.employee.find()
    {
         "_id": {
         "$oid": "516a76a1c9565daf06030000"
         },
         "age": 11,
         "name": "Mike"
    }
    {
         "_id": {
         "$oid": "516a76a1c9565daf06050000"
         },
         "a": 10,
         "age": 22,
         "name":"Mike"
    }
    Return 2 row(s).
    ```

* Select the records that meet the matching conditions, and update these records according to the update rules, which is set the "rule" and "cond" parameters. The following operation uses [$exists][exists] to match records with the "type" field, and uses [$inc][inc] to increase the "age" field value of these records by 3. In the two records given above, there is no "type" field. At this time, the upsert operation will insert a new record. The new record only has the "\_id" field and the "age" field name. The "\_id" field value will be automatically generated, and the "age" field value Is 3.

    ```lang-javascript
    > db.sample.employee.upsert({$inc: {age: 3}}, {type: {$exists: 1}})
    {
      "UpdatedNum": 0,
      "ModifiedNum": 0,
      "InsertedNum": 1
    }
    >
    > db.sample.employee.find()
    {
         "_id": {
         "$oid": "516a76a1c9565daf06030000"
         },
         "age": 11,
         "name": "Mike"
    }
    {
         "_id": {
         "$oid": "516a76a1c9565daf06050000"
         },
         "a": 10,
         "age": 22,
         "name":"Mike"
    }
    {
         "_id": {
         "$oid": "516cfc334630a7f338c169b0"
         },
         "age": 3
    }
    Return 3 row(s).
    ```

* Update records by access plan. Assuming that the specified index name "testIndex" exists in the collection, this operation is equivalent to using the update method, using the index named "testIndex" to access the records whose "age" field value is greater than 20 in the collection "sample.employee", and adding 1 to the "age" field name of these records.

    ```lang-javascript
    > db.sample.employee.upsert({$inc: {age: 1}}, {age: {$gt: 20}}, {"": "testIndex"})
    {
      "UpdatedNum": 1,
      "ModifiedNum": 1,
      "InsertedNum": 0
    }
    >
    > db.sample.employee.find()
    {
         "_id": {
         "$oid": "516a76a1c9565daf06050000"
         },
         "a": 10,
         "age": 23,
         "name":"Mike"
    }
    Return 1 row(s).
    ```

* Use setOnInsert to update the record. Since the records with the "age" field value greater than 30 in the collection employee are empty, upsert appends the field {"name":"Mike"} to the inserted record when doing the insert operation.

    ```lang-javascript
    > db.sample.employee.upsert({$inc: {age: 1}}, {age: {$gt: 30}}, {}, {"name": "Mike"})
    {
      "UpdatedNum": 0,
      "ModifiedNum": 0,
      "InsertedNum": 1
    }
    >
    > db.sample.employee.find({"age": 1, "name": "Mike"})
    {
         "_id": {
         "$oid": "516a76a1c9565daf06050000"
         },
         "age":1,
         "name":"Mike"
    } 
    Return 1 row(s).
    ```

* The partition collection "sample.employee" and the partition key is {a: 1}. It contains the following records.

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
 
    Specify the "KeepShardingKey" parameter: Do not keep the partition key field in the update rule. Only the non-partition key "b" field is updated, and the value of the partition key "a" field is not updated.
 
    ```lang-javascript
    > db.sample.employee.upsert({$set: {a: 9, b: 9}}, {}, {}, {}, {KeepShardingKey: false})
    {
      "UpdatedNum": 1,
      "ModifiedNum": 1,
      "InsertedNum": 0
    }
    >
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
 
    Specify the "KeepShardingKey" parameter: Keep the partition key field in the update rule. Since updating the partition key is currently not supported, an error will be reported.
 
    ```lang-javascript
    > db.sample.employee.upsert({$set: {a: 9}}, {}, {}, {}, {KeepShardingKey: true})
    (nofile):0 uncaught exception: -178
    Sharding key cannot be updated
    ```


[^_^]:
     Links
[find]:manual/Manual/Sequoiadb_Command/SdbCollection/find.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[inc]:manual/Manual/Operator/Update_Operator/inc.md
[exists]:manual/Manual/Operator/Match_Operator/exists.md
[set]:manual/Manual/Operator/Update_Operator/set.md
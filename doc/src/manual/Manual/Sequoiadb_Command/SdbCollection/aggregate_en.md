##NAME##

aggregate - Calculate the aggregate value of the data in the collection

##SYNOPSIS##

**db.collectionspace.collection.aggregate( \<subOp1\>，[subOp2]，... )**

##CATEGORY##

Collection

##DESCRIPTION##

The functions of aggregate() and find() are pretty similar to each other.  It also retrieves the documents from the collection in the SequoiaDB and return a cursor.

##PARAMETERS##

* `subOp` ( *Object*, *Required* )

    subOp1,subOp2... means sub-operations which containing aggregate operators. 1 to N sub-operations can be filled in the aggregate() method. Every sub-operation is an object which contatining aggregate operators, and sub-operations be separated by commans.

    >**Note:**
    > 
    > The aggregate() method will perform each sub-operation from left to right in the order of sub-operations.

##RETURN VALUE##

On success, aggregate() returns an object of DBCursor for iterating the result.

On error, exception will be thrown.

##ERRORS##

the exceptions of `aggregate()` are as below:

| Error code | Error type | Description | Solution |
| ------ | ------ | --- | ------ |
| -6 | SDB_INVALIDARG | Invalid Argument. | Check whether the input arguments are valid or not. |

When the exception happens, use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get the error message or use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the error code. For more details, refer to [Troubleshooting](troubleshooting/general/general_guide.md).

##HISTORY##

Since v1.0

##EXAMPLES##

Assume a collection stores records in following format:

```lang-json
{
  no:1000,
  score:80,
  interest:["basketball","football"],
  major:"Computer Science and Technology",
  dep:"Computer Academy",
  info:
  {
    name:"Tom",
    age:25,
    gender:"male"
  }
}
```

1. Select records according to the criteria, and specify the returned field name.

    ```lang-javascript
    > db.sample.employee.aggregate( { $match: { $and: [ { no: { $gt: 1002 } },
                                                { no: { $lt: 1015 } },
                                                { dep: "Computer Academy" } ] } },
                                  { $project: { no: 1, "info.name": 1, major: 1 } } )
    {
        "no": 1003,
        "info.name": "Sam",
        "major": "Computer Software and Theory"
    }
    {
          "no": 1004,
          "info.name": "Coll",
          "major": "Computer Engineering"
    }
    {
          "no": 1005,
          "info.name": "Jim",
          "major": "Computer Engineering"
    }
    ```

2. Select records by criteria and divide the selected records into groups. This operation firstly uses $match to select records that match the selecting criteria, then uses $group to divide the selected records by field "major", and uses $avg to return the average of the "age" field in each group

    ```lang-javascript
    > db.sample.employee.aggregate( { $match: { dep:  "Computer Academy" } },
                                  { $group: { _id:  "$major", Major: { $first: "$major" }, 
                                  avg_age: { $avg: "$info.age" } } } ) 
    {
        "Major": "Computer Engineering",
        "avg_age": 25
    }
    {
          "Major": "Computer Science and Technology",
          "avg_age": 22.5
    }
    {
          "Major": "Computer Software and Theory",
          "avg_age": 26
    }
    ```

3. Select records by criteria, then group and sort the selected records, limit the starting point of the result set and the number of returned records. This aggregate operation firstly uses $match to select records match the criteria, then uses $group to group the records by "major", uses $avg to return the average of "age" field in each group, and sort the records in descending order, uses $skip to specify the starting point of the returned result, and $limit to limit the number of returned records.

    ```lang-javascript
    > db.sample.employee.aggregate( { $match: { interest: { $exists: 1 } } }, 
                                  { $group: { _id: "$major", 
                                              avg_age: { $avg: "$info.age" }, 
                                              major: { $first: "$major" } } }, 
                                  { $sort: { avg_age: -1, major: -1 } }, 
                                  { $skip: 2 }, 
                                  { $limit: 3 } )
    {
        "avg_age": 25,
        "major": "Computer Science and Technology"
    }
    {
          "avg_age": 22,
          "major": "Computer Software and Theory"
    }
    {
          "avg_age": 22,
          "major": "Physics"
    }
    ```

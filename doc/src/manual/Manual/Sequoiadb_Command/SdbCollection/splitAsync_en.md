##NAME##

splitAsync - split data records asynchronously

##SYNOPSIS##

**db.collectionspace.collection.splitAsync(\<source group\>, \<target group\>, \<percent\>)**

**db.collectionspace.collection.splitAsync(\<source group\>, \<target group\>, \<condition\>, [endcondition])**

##CATEGORY##

SdbCollection

##DESCRIPTION##

This function is used to split the data records in the source partition group into the target partition group asynchronously according to conditions. The source partition group and the target partition group must belong to the same domain.

##PARAMETERS##

###Range split###

db.collectionspace.collection.splitAsync(\<source group\>, \<target group\>, \<condition\>, [endcondition])

| Name | Type| Description | Required or not |
| ------ | -------- | ------ | -------- |
| source group | string | Source partition group | required |
| target group | string | Target partition group | required |
| condition | object  | Range split condition | required |
| endcondition | object | End range condition| not |

> **Note:**
>
> - "Range" partitioning uses precise conditions, while "Hash" partitioning uses Partition (number of partitions) conditions. When the end condition is not selected, the default is the maximum data range currently contained in the segmentation source.
> - If the specified partition key field is in descending order, such as {groupingKey: {<field 1>: &lt; -1&gt;}}, condition (or Partition), The range in the start condition in the above example should be greater than the range in the end condition. The Partition (number of partitions) used by the "Hash” partition must be an integer, not other types.

###Percentage split###

db.collectionspace.collection.splitAsync(\<source group\>, \<target group\>, \<percent\>)

| Name | Type| Description | Required or not |
| ------ | -------- | ------ | -------- |
| source group | string | Source partition group | required |
| target group | string | Target partition group | required |
| percent | double | Percentage split condition, Value: (0, 100] | required |

> **Note:**
>
> - Range partition needs to ensure that the source partition group contains data, which means the collection is not empty.
> - The percentage cannot be 0.

##RETURN VALUE##

When the function executes successfully, it will return an object of type Number. Users can use the task ID to operate the task.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

* Hash partition range split.

    ```lang-javascript
    > db.sample.employee.splitAsync("group1", "group2", {Partition: 10}, {Partition: 20})
    3
    ```

* Range partition range split.

    ```lang-javascript
    > db.sample.employee.splitAsync("group1", "group2", {a: 10}, {a: 10000})
    2
    ```

* Percentage split.

    ```lang-javascript
    > db.sample.employee.splitAsync("group1", "group2", 50) 
    5
    ```


[^_^]:
     Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md

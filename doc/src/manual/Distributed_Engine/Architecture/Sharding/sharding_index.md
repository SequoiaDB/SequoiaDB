每一个数据库分区集合都会默认创建一个名叫“$shard”的索引，该索引称为分区索引。

分区索引存在于数据库分区集合所在的每一个分区组中，其字段定义顺序和排列与分区键相同。

>**Note:**
>
> - 非分区集合不存在分区索引。
> - 任何用户定义的唯一索引必须包含分区索引中“Key”的所有字段。
> - 在分区集合中，_id 字段仅保证分区内该字段唯一，无法保证全局唯一。

##示例##

一个典型的分区索引如下：

```lang-javascript
> db.sample.employee.listIndexes()
{
  "IndexDef": 
  {
    "name": "$shard",
    "_id": { "$oid": "515954bfa88873112fa6bd3a" },
    "key": { "Field1": 1, "Field2": -1 },
    "v": 0,
    "unique": false,
    "dropDups": false,
    "enforced": false
  },
  "IndexFlag": "Normal"
}
```

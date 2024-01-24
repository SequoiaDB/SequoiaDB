

如下是描述 SQL 关键字与 SequoiaDB 聚集操作符的对照表：

| SQL 关键字 | SequoiaDB 聚集操作符 |
| --------- | ------------------- |
| where     | $match              |
| group by  | $group              |
| having    | 对 $group 后的字段进行 $match |
| select    | $project            |
| order by  | $sort               |
| top 或者 limit | $limit         |
| offset    | $skip               |

如下是描述标准 SQL 语句与 SequoiaDB 聚集语句之间的对照表

| SQL 语句 | SequoiaDB 语句 | 描述 |
| ------- | ------------- | ---- |
| select product_id as p_id , price from sample.employee | db.sample.employee.aggregate({$project:{p_id:"$product_id",price:1,date:0}}) | 返回所有记录的 product_id 和 price 字段，其中 product_id 重命名为 p_id，对记录中的 date 字段不返回。 |
| select sum(price) as total from sample.employee | db.sample.employee.aggregate({$group:{_id:null,total:{$sum:"$price"}}}) | 对 table 中的字段 price 值求和，并重命名为 total。 |
| select product_id, sum(price) as total from sample.employee group by product_id | db.sample.employee.aggregate({$group:{ _id:"$product_id",product_id:{$first:"$product_id"},total:{$sum:"$price"}}}) | 对表 table 中的记录按 product_id 字段分组；求每个分组中字段 price 值的累加和，并重命名为 total。|
| select product_id, sum(price) as total from sample.employee group by product_id order by total | db.sample.employee.aggregate({$group:{_id:"$product_id",product_id:{$first:"$product_id"},total:{$sum:"$price"}}},{$sort:{total:1}}) | 对表 table 中的记录按 product_id 字段分组；求每个分组中字段 price 值的累加和，并重命名为 total；对结果集按字段名 total 的值升序排序。 |
| select product_type_id, product_id, sum(price) as total from sample.employee group by product_type_id, product_id | db.sample.employee.aggregate({$group:{ _id:{product_type_id:"$product_type_id",product_id:"$product_id"},product_id:{$first:"$product_id"},total:{$sum:"$price"}}}) | 对表 table 中的记录按首先按 product_type_id 字段分组，再按 product_id 字段分组；求每个分组中字段 price 值的累加和，并重命名为 total。 |
| select product_id, sum(price) as total from sample.employee group by product_id having total > 1000 | db.sample.employee.aggregate({$group:{_id:"$product_id",product_id:{$first:"$product_id"},total:{$sum:"$price"}}},{$match:{total:{$gt:1000}}}) | 对表 table 中的记录按 product_id 字段分组；求每个分组中字段 price 值的累加和，并重命名为 total；只返回满足条件 total 字段值大于1000的分组。 |
| select product_id, sum(price) as total from sample.employee where product_type_id = 1001 group by product_id | db.sample.employee.aggregate({$match:{product_type_id:1001}},{$group:{\_id:"$product_id",product_id:{$first:"$product_id"},total:{$sum:"$price"}}}) | 选择符合条件 product_type_id = 1001 的记录；对选出的记录按 product_id 进行分组；对每个分组中的 price 字段值就和，并重命名为 total。 |
| select product_id, sum(price) as total from sample.employee where product_type_id = 1001 group by product_id having total > 1000 | db.sample.employee.aggregate({$match:{product_type_id:1001}},{$group:{_id:"$product_id",product_id:{$first:"$product_id"},total:{$sum:"$price"}}},{$match:{total:{$gt:1000}}}) | 选择符合条件 product_type_id = 1001 的记录；对选出的记录按 product_id 进行分组；对每个分组中的 price 字段值就和，并重命名为 total；只返回满足条件 total 字段值大于1000的分组。 |
| select top 10 * from sample.employee | db.sample.employee.aggregate({$group:{_id:null}},{$limit:10}) | 返回结果集中的前10条记录。|
| select * from sample.employee offset 50 rows fetch next 10 | db.sample.employee.aggregate({$group:{_id:null}},{$skip:50},{$limit:10}) | 跳过结果集中前50条记录之后，返回接下来的10条记录。 |

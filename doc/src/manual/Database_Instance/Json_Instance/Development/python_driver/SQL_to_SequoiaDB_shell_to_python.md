
SequoiaDB 巨杉数据库的查询用 dict（bson）对象表示，下表以例子的形式显示了 SQL 语句、SDB Shell 语句和 SequoiaDB Python 驱动程序语法之间的对照。

| SQL                                                | SDB Shell                                            | Python Driver                                                |
| -------------------------------------------------- | ---------------------------------------------------------- | ------------------------------------------------------------ |
| insert into employee(a,b) values(1,-1)             | db.sample.employee.insert({a:1,b:-1})                      | cl = db.get_collection("sample.employee")<br>obj = { "a":1, "b":-1 }<br>cl.insert( obj ) |
| select a,b from employee                           | db.sample.employee.find(null,{a:"",b:""})                  | cl = db.get_collection("sample.employee")<br>selected = { "a":"","b":"" }<br>cr = cl.query(selector = selected ) |
| select * from employee                             | db.sample.employee.find()                                  | cl = db.get_collection("sample.employee")<br>cr = cl.query () |
| select * from employee where age=20                | db.sample.employee.find({age:20})                          | cl = db.get_collection("sample.employee")<br>cond ={"age":20}<br>cr = cl.query ( condition = cond ) |
| select * from employee where age=20 order by name  | db.sample.employee.find({age:20}).sort({name:1})           | cl = db.get_collection("sample.employee")<br>cond ={"age":20}<br>orderBy = {"name":1}<br>cr = cl.query (condition=cond , order_by=orderBy) |
| select * from employee where age > 20 and age < 30 | db.sample.employee.find({age:{$gt:20,$lt:30}})             | cl = db.get_collection("sample.employee")<br>cond = {"age":{"$gt":20,"$lt":30}}<br>cr = cl.query (condition = cond ) |
| create index testIndex on employee(name)           | db.sample.employee.createIndex("testIndex",{name:1},false) | cl = db.get_collection("sample.employee")<br>obj = { "name":1 }<br>cl.create_index ( obj, "testIndex", False, False ) |
| select * from employee limit 20 offset 10          | db.sample.employee.find().limit(20).skip(10)               | cl = db.get_collection("sample.employee")<br>cr = cl.query (num_to_skip=10L, num_to_return=20L ) |
| select count(*) from employee where age > 20       | db.sample.employee.find({age:{$gt:20}}).count()            | cl = db.get_collection("sample.employee")<br>count = 0L<br>condition = { "age":{"$gt":20}}<br>count = cl.get_count (condition ) |
| update employee set a=2 where b=-1                 | db.sample.employee.update({$set:{a:2}},{b:-1})             | cl = db.get_collection("sample.employee")<br>condition = { "b":1 }<br>rule = { "$set":{"a":2} }<br>cl.update ( rule, condition=condition ) |
| delete from employee where a=1                     | db.sample.employee.remove({a:1})                           | cl = db.get_collection("sample.employee")<br>condition = {"a":1}<br>cl.delete ( condition=condition ) |

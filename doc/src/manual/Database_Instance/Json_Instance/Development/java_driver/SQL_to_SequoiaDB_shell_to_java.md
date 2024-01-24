
SequoiaDB 巨杉数据库的查询用 json（bson）对象表示，下表以示例的形式展示 SQL 语句、SDB Shell 语句和 SequoiaDB Java 驱动程序语法之间的对照。


| SQL                                                | SDB Shell                                              | Java Driver                                                  |
| -------------------------------------------------- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| insert into employee( a, b) values( 1, -1 )        | db.sample.employee.insert( { a: 1, b: -1 } )                 | employee.insert( "{ 'a': 1, 'b': -1 }" )                     |
| select a,b from employee                           | db.sample.employee.find( null, { a: "", b: "" } )            | employee.query( "", "{ 'a': '', 'b': '' }", "", "" )         |
| select * from employee                             | db.sample.employee.find()                                    | employee.query()                                             |
| select * from employee where age=20                | db.sample.employee.find( { age: 20 } )                       | employee.query( "{ 'age': 20 }", "", "", "")                 |
| select * from employee where age=20 order by name  | db.sample.employee.find( { age: 20 } ).sort( { name: 1 } )   | employee.query( "{ 'age': 20 }", "", "{ 'name': 1 }", "" )   |
| select * from employee where age > 20 and age < 30 | db.sample.employee.find( { age: { $gt: 20, $lt: 30 } } )     | employee.query( "{ 'age': { '$gt': 20, '$lt': 30 } }", "", "", "") |
| create index testIndex on employee(name)           | db.sample.employee.createIndex( "testIndex", { name: 1 }, false ) | employee.createIndex( "testIndex", "{ 'name': 1 }", false, false ) |
| select * from employee limit 20 offset 10          | db.sample.employee.find().limit( 20 ).skip( 10 )             | employee.query( "", "", "", "", 10, 20 )                     |
| select count(*) from employee where age > 20       | db.sample.employee.find( { age: { $gt: 20 } } ).count()      | employee.getCount( "{ 'age': { '$gt': 20 } }" )              |
| update employee set a=a+2 where b=-1               | db.sample.employee.update( { $inc: { a: 2 } }, { b: -1 } )   | employee.update( "{ 'b': -1 }", "{ '$inc': { 'a': 2 } }", "" ) |
| delete from employee where a=1                     | db.sample.employee.remove( { a: 1 } )                        | employee.delete( "{ 'a': 1 }" )                              |





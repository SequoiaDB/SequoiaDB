本文档主要介绍 SequoiaDB 巨杉数据库的术语及部分 SQL 操作对应的 API 语句。

##概念和术语##

| SQL                                      | API                                                          |
| ---------------------------------------- | ------------------------------------------------------------ |
| database                                 | database                                                     |
| tableSpace                               | collection space                                             |
| table                                    | collection                                                   |
| row                                      | document / BSON document                                     |
| column                                   | field                                                        |
| index                                    | index                                                        |
| table joins                              | embedded documents                                           |
| primary key （指定任何唯一的列作为主键） | primary key （在 SequoiaDB 中，primary key 是自动创建到记录的 \_id 字段名中） |

##Create和Alter##

下表列出了各种 SQL 语句表级别的操作和在 API 中对应的操作：

| SQL 语句 | API 语句                                                     |
| -------- | -------------- |
| create table student ( id not null, stu_id varchar(50), age number primary key(id) ) | 如果未指定 \_id 字段，\_id 字段自动添加 db.collectionspace.student( { stu_id: "01", age: 20 } )，当然你也可以明确的创建一个集合 db.collectionspace.createCL( "student" ) |
| alter table student add name varchar(50) | 集合不描述或强制执行文档的结构，即在集合上没有结构的改动操作，但是 update() 方法可以使用 $set 向文档记录添加不存在的字段。db.collectionspace.student.update( {}, { $set: { name: "Tom" } } ) |
| alter table student drop column name |  集合不描述或强制执行文档的结构，即在集合上没有结构的改动操作，但是 update() 方法可以使用 $unset 向文档记录删除存在的字段。db.collectionspace.student.update( {}, { $unset: { name: "Tom" } } ) |
| create index index_stu_id on student ( stu_id )                     |  db.collectionspace.student.createIndex( "index_stu_id", { stu_id: -1 } )  |
| drop table student | db.collectionspace.dropCL( "student" )         |

##Insert##

下表给出了各种 SQL 语句在表级上的插入操作和 API上相应的操作：

| SQL 语句                                              | API 语句                                                     |
| ----------------------------------------------------- | ------------------------------------------------------------ |
| insert into student( stu_id, age ) values( "01", 20 ) | db.collectionspace.student.insert( { stu_id: "01", age: 20 } ) |

##Select##

下表给出了各种 SQL 语句在表级上的读操作和 API 上相应的操作：


| SQL 语句                        | API 语句                            |
|-------------------------------- |----------------------------------------------- |
| select * from student           | db.collectionspace.student.find()              |
| select stu_id, age from student | db.collectionspace.student.find( {},{ stu_id: "01", age: 20 } )                  |
| select * from student where age > 25 | db.collectionspace.student.find( { age: { $gt: 25 } } )            |
| select age from student where age = 25 and stu_id= "01" | db.collectionspace.student.find( { age: 25, stu_id: "01" }, { age: 25 } ) |
| select count( * ) from student                            | db.collectionspace.student.count()                             |
| select count( stu_id ) from student                       | db.collectionspace.student.count( { stu_id: { $exists: 1 } } )     |


##Update##

下表给出了各种 SQL 语句在表级上的更新操作和 API上相应的操作：

| SQL 语句                                             | API 语句                                                     |
| ---------------------------------------------------- | ------------------------------------------------------------ |
| update student set age = 25 where stu_id = "01"      | db.collectionspace.student.update( { stu_id: "01" },{ $set: { age: 25 } } ) |
| update student set age = age + 2 where stu_id = "01" | db.collectionspace.student.update( { stu_id: "01" },{ $inc: { age: 2 } } )     , |

##Delete##

下表给出了各种 SQL 语句在表级上的删除记录操作和 API 上相应的操作：

| SQL 语句 | API 语句                                         |
| ---------------------------------- | ------------------------------------------- |
| delete from student where age = 20 | db.collectionspace.student.remove( { age: 20 } ) |
| delete from student                | db.collectionspace.student.remove()         |

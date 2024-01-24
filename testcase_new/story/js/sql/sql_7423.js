/******************************************************************************
@Description  seqDB-7423: 内置SQL语法中的子句的单表测试
                        1. where、limit、offset
                        2. as，集合别名、字段别名、结果集别名
                        3. group by、order by
                        4. split by
@author liyuanyue
@date 2020-4-8
******************************************************************************/
testConf.clName = COMMCLNAME + "_7423";

main( test );

function test ()
{
   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_7423";

   db.execUpdate( "insert into " + csName + "." + clName + " (name,age) values (\"Tom\",1)" );
   db.execUpdate( "insert into " + csName + "." + clName + " (name,age) values (\"Mike\",2)" );
   db.execUpdate( "insert into " + csName + "." + clName + " (name,age) values (\"Lisa\",3)" );
   db.execUpdate( "insert into " + csName + "." + clName + " (name,age) values (\"Json\",4)" );
   db.execUpdate( "insert into " + csName + "." + clName + " (name,age) values (\"Jhon\",5)" );
   db.execUpdate( "insert into " + csName + "." + clName + " (name,age) values (\"Tina\",6)" );
   db.execUpdate( "insert into " + csName + "." + clName + " (name,age) values (\"Pite\",7)" );

   // where、limit、offset
   var rc = db.exec( "select * from " + csName + "." + clName + " where age > 3 offset 2 limit 1" );
   var expRecs = [{ "name": "Tina", "age": 6 }];
   commCompareResults( rc, expRecs );

   // as，集合别名、字段别名、结果集别名
   var rc = db.exec( "select T.age from " + csName + "." + clName + " as T where T.age < 2" );
   var expRecs = [{ "age": 1 }];
   commCompareResults( rc, expRecs );

   var rc = db.exec( "select name as _name from " + csName + "." + clName + " where age = 2" );
   var expRecs = [{ "_name": "Mike" }];
   commCompareResults( rc, expRecs );

   var rc = db.exec( "select T.age,T.name from (select age,name from " + csName + "." + clName + ") as T where T.age < 4" );
   var expRecs = [{ "name": "Tom", "age": 1 }, { "name": "Mike", "age": 2 }, { "name": "Lisa", "age": 3 }];
   commCompareResults( rc, expRecs );

   db.execUpdate( "delete from " + csName + "." + clName );

   // group by、order by
   db.execUpdate( "insert into " + csName + "." + clName + " (emp_no,dept_no) values (1,1)" );
   db.execUpdate( "insert into " + csName + "." + clName + " (emp_no,dept_no) values (2,1)" );
   db.execUpdate( "insert into " + csName + "." + clName + " (emp_no,dept_no) values (3,2)" );
   db.execUpdate( "insert into " + csName + "." + clName + " (emp_no,dept_no) values (4,2)" );

   var rc = db.exec( "select dept_no,count(emp_no) as emp from " + csName + "." + clName + " group by dept_no order by dept_no desc" );
   var expRecs = [{ "dept_no": 2, "emp": 2 }, { "dept_no": 1, "emp": 2 }];
   commCompareResults( rc, expRecs );

   db.execUpdate( "delete from " + csName + "." + clName );

   // split by
   testPara.testCL.insert( { a: 1, b: 2, c: [3, 4, 5] } );
   testPara.testCL.insert( { a: 2, b: 3, c: [6, 7] } );

   var rc = db.exec( "select a,b,c from " + csName + "." + clName + " split by c" );
   var expRecs = [
      { "a": 1, "b": 2, "c": 3 }, { "a": 1, "b": 2, "c": 4 }, { "a": 1, "b": 2, "c": 5 },
      { "a": 2, "b": 3, "c": 6 }, { "a": 2, "b": 3, "c": 7 }
   ];
   commCompareResults( rc, expRecs );

   db.execUpdate( "delete from " + csName + "." + clName );
}
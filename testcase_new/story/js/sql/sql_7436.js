/******************************************************************************
@Description seqDB-7436: 内置SQL语法中的函数测试
                        1. sum()
                        2. count()
                        3. avg()
                        4. max()
                        5. min()
                        6. first()
                        7. last()
                        8. push()
                        9. addtoset()
                        10. buildobj()
                        11. mergearrayset()
@author liyuanyue
@date 2020-4-9
******************************************************************************/
testConf.clName = COMMCLNAME + "_7436";

main( test );

function test ()
{
   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_7436";

   db.execUpdate( "insert into " + csName + "." + clName + " (name,age) values (\"Jhon\",33)" );
   db.execUpdate( "insert into " + csName + "." + clName + " (name,age) values (\"Tom\",5)" );
   db.execUpdate( "insert into " + csName + "." + clName + " (name,age) values (\"Mike\",7)" );
   db.execUpdate( "insert into " + csName + "." + clName + " (name,age) values (\"Lisa\",8)" );
   db.execUpdate( "insert into " + csName + "." + clName + " (name,age) values (\"Json\",9)" );
   db.execUpdate( "insert into " + csName + "." + clName + " (name,age) values (\"Pite\",87)" );
   db.execUpdate( "insert into " + csName + "." + clName + " (name,age) values (\"Tina\",26)" );

   // sum()
   var rc = db.exec( "select sum(age) as sum_age from " + csName + "." + clName );
   var expRecs = [{ "sum_age": 175 }];
   commCompareResults( rc, expRecs );

   // count()
   var rc = db.exec( "select count(age) as count_age from " + csName + "." + clName );
   var expRecs = [{ "count_age": 7 }];
   commCompareResults( rc, expRecs );

   // avg()
   var rc = db.exec( "select avg(age) as avg_age from " + csName + "." + clName );
   var expRecs = [{ "avg_age": 25 }];
   commCompareResults( rc, expRecs );

   // max()
   var rc = db.exec( "select max(age) as max_age from " + csName + "." + clName );
   var expRecs = [{ "max_age": 87 }];
   commCompareResults( rc, expRecs );

   // min()
   var rc = db.exec( "select min(age) as min_age from " + csName + "." + clName );
   var expRecs = [{ "min_age": 5 }];
   commCompareResults( rc, expRecs );

   // first()
   var rc = db.exec( "select first(age) as first_age from " + csName + "." + clName );
   var expRecs = [{ "first_age": 33 }];
   commCompareResults( rc, expRecs );

   // last()
   var rc = db.exec( "select last(age) as last_age from " + csName + "." + clName );
   var expRecs = [{ "last_age": 26 }];
   commCompareResults( rc, expRecs );

   db.execUpdate( "delete from " + csName + "." + clName );

   db.execUpdate( "insert into " + csName + "." + clName + " (a,b) values (1,1)" );
   db.execUpdate( "insert into " + csName + "." + clName + " (a,b) values (2,2)" );
   db.execUpdate( "insert into " + csName + "." + clName + " (a,b) values (2,2)" );
   db.execUpdate( "insert into " + csName + "." + clName + " (a,b) values (2,3)" );

   // push()
   var rc = db.exec( "select a, push(b) as b from " + csName + "." + clName + " group by a" );
   var expRecs = [{ "a": 1, "b": [1] }, { "a": 2, "b": [2, 2, 3] }];
   commCompareResults( rc, expRecs );

   // addtoset()
   var rc = db.exec( "select a, addtoset(b) as b from " + csName + "." + clName + " group by a" );
   var expRecs = [{ "a": 1, "b": [1] }, { "a": 2, "b": [2, 3] }];
   commCompareResults( rc, expRecs );

   // buildobj()
   var rc = db.exec( "select buildobj(a,b) as a_b from " + csName + "." + clName );
   var expRecs = [{ "a_b": { "a": 1, "b": 1 } }, { "a_b": { "a": 2, "b": 2 } }, { "a_b": { "a": 2, "b": 2 } }, { "a_b": { "a": 2, "b": 3 } }];
   commCompareResults( rc, expRecs );

   // mergearrayset()
   var tmpclName = COMMCLNAME + "_7436_tmp";
   commDropCL( db, csName, tmpclName );
   var tmpcl = commCreateCL( db, csName, tmpclName, {}, false );

   tmpcl.insert( { a: 1, b: [1, 2] } );
   tmpcl.insert( { a: 1, b: [2, 3] } );

   var rc = db.exec( "select a, mergearrayset(b) as b from " + csName + "." + tmpclName + " group by a" );
   var expRecs = [{ "a": 1, "b": [1, 2, 3] }];
   commCompareResults( rc, expRecs );

   commDropCL( db, csName, tmpclName );
}
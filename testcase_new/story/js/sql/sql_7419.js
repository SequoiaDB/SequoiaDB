/******************************************************************************
@Description seqDB-7419:select 使用算术表达式和正则表达式查询
@author liyuanyue
@date 2020-4-3
******************************************************************************/
testConf.clName = COMMCLNAME + "_7419";

main( test );

function test ()
{
   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_7419";

   // 使用算术表达式查询
   for( var i = 0; i < 20; i++ )
   {
      db.execUpdate( "insert into " + csName + "." + clName + " (name,age) values (\"Tom\"," + i + ")" );
   }

   var rc = db.exec( "select * from " + csName + "." + clName );
   var expRecs = [
      { "name": "Tom", "age": 0 }, { "name": "Tom", "age": 1 },
      { "name": "Tom", "age": 2 }, { "name": "Tom", "age": 3 },
      { "name": "Tom", "age": 4 }, { "name": "Tom", "age": 5 },
      { "name": "Tom", "age": 6 }, { "name": "Tom", "age": 7 },
      { "name": "Tom", "age": 8 }, { "name": "Tom", "age": 9 },
      { "name": "Tom", "age": 10 }, { "name": "Tom", "age": 11 },
      { "name": "Tom", "age": 12 }, { "name": "Tom", "age": 13 },
      { "name": "Tom", "age": 14 }, { "name": "Tom", "age": 15 },
      { "name": "Tom", "age": 16 }, { "name": "Tom", "age": 17 },
      { "name": "Tom", "age": 18 }, { "name": "Tom", "age": 19 }];
   commCompareResults( rc, expRecs );
   var rc = db.exec( "select age from " + csName + "." + clName );
   var expRecs = [
      { "age": 0 }, { "age": 1 }, { "age": 2 }, { "age": 3 },
      { "age": 4 }, { "age": 5 }, { "age": 6 }, { "age": 7 },
      { "age": 8 }, { "age": 9 }, { "age": 10 }, { "age": 11 },
      { "age": 12 }, { "age": 13 }, { "age": 14 }, { "age": 15 },
      { "age": 16 }, { "age": 17 }, { "age": 18 }, { "age": 19 }];
   commCompareResults( rc, expRecs );
   var rc = db.exec( "select * from " + csName + "." + clName + " where age>10" );
   var expRecs = [
      { "name": "Tom", "age": 11 },
      { "name": "Tom", "age": 12 }, { "name": "Tom", "age": 13 },
      { "name": "Tom", "age": 14 }, { "name": "Tom", "age": 15 },
      { "name": "Tom", "age": 16 }, { "name": "Tom", "age": 17 },
      { "name": "Tom", "age": 18 }, { "name": "Tom", "age": 19 }];
   commCompareResults( rc, expRecs );
   var rc = db.exec( "select _id,age from " + csName + "." + clName + " where age<10" );
   var expRecs = [
      { "age": 0 }, { "age": 1 }, { "age": 2 }, { "age": 3 }, { "age": 4 },
      { "age": 5 }, { "age": 6 }, { "age": 7 }, { "age": 8 }, { "age": 9 }];
   commCompareResults( rc, expRecs );
   var rc = db.exec( "select name,age from " + csName + "." + clName + " where age=10" );
   var expRecs = [{ "name": "Tom", "age": 10 }];
   commCompareResults( rc, expRecs );
   var rc = db.exec( "select name,age from " + csName + "." + clName + " where age>=10" );
   var expRecs = [
      { "name": "Tom", "age": 10 }, { "name": "Tom", "age": 11 },
      { "name": "Tom", "age": 12 }, { "name": "Tom", "age": 13 },
      { "name": "Tom", "age": 14 }, { "name": "Tom", "age": 15 },
      { "name": "Tom", "age": 16 }, { "name": "Tom", "age": 17 },
      { "name": "Tom", "age": 18 }, { "name": "Tom", "age": 19 }];
   commCompareResults( rc, expRecs );
   var rc = db.exec( "select name,age from " + csName + "." + clName + " where age<=10" );
   var expRecs = [
      { "name": "Tom", "age": 0 }, { "name": "Tom", "age": 1 },
      { "name": "Tom", "age": 2 }, { "name": "Tom", "age": 3 },
      { "name": "Tom", "age": 4 }, { "name": "Tom", "age": 5 },
      { "name": "Tom", "age": 6 }, { "name": "Tom", "age": 7 },
      { "name": "Tom", "age": 8 }, { "name": "Tom", "age": 9 },
      { "name": "Tom", "age": 10 }];
   commCompareResults( rc, expRecs );
   var rc = db.exec( "select * from " + csName + "." + clName + " where name=\"Tom\" and age<>5" );
   var expRecs = [
      { "name": "Tom", "age": 0 }, { "name": "Tom", "age": 1 },
      { "name": "Tom", "age": 2 }, { "name": "Tom", "age": 3 },
      { "name": "Tom", "age": 4 },
      { "name": "Tom", "age": 6 }, { "name": "Tom", "age": 7 },
      { "name": "Tom", "age": 8 }, { "name": "Tom", "age": 9 },
      { "name": "Tom", "age": 10 }, { "name": "Tom", "age": 11 },
      { "name": "Tom", "age": 12 }, { "name": "Tom", "age": 13 },
      { "name": "Tom", "age": 14 }, { "name": "Tom", "age": 15 },
      { "name": "Tom", "age": 16 }, { "name": "Tom", "age": 17 },
      { "name": "Tom", "age": 18 }, { "name": "Tom", "age": 19 }];
   commCompareResults( rc, expRecs );
   var rc = db.exec( "select * from " + csName + "." + clName + " where age>10 and age<15 order by age desc" );
   var expRecs = [
      { "name": "Tom", "age": 14 }, { "name": "Tom", "age": 13 },
      { "name": "Tom", "age": 12 }, { "name": "Tom", "age": 11 }];
   commCompareResults( rc, expRecs );
   var rc = db.exec( "select * from " + csName + "." + clName + " where age<5 or age>15 order by age asc" );
   var expRecs = [
      { "name": "Tom", "age": 0 }, { "name": "Tom", "age": 1 },
      { "name": "Tom", "age": 2 }, { "name": "Tom", "age": 3 },
      { "name": "Tom", "age": 4 },
      { "name": "Tom", "age": 16 }, { "name": "Tom", "age": 17 },
      { "name": "Tom", "age": 18 }, { "name": "Tom", "age": 19 }];
   commCompareResults( rc, expRecs );

   db.execUpdate( "delete from " + csName + "." + clName );

   // 使用正则表达式查询
   db.execUpdate( "insert into " + csName + "." + clName + " (name) values('Aom1') " );
   db.execUpdate( "insert into " + csName + "." + clName + " (name) values('Aom2') " );
   db.execUpdate( "insert into " + csName + "." + clName + " (name) values('aom3') " );
   db.execUpdate( "insert into " + csName + "." + clName + " (name) values('Bom4') " );
   db.execUpdate( "insert into " + csName + "." + clName + " (name) values('Tom5') " );
   db.execUpdate( "insert into " + csName + "." + clName + " (name) values('CCC6') " );
   db.execUpdate( "insert into " + csName + "." + clName + " (name) values('D 7') " );

   var rc = db.exec( "select * from " + csName + "." + clName + " where name like '^A.*1$' " );
   var expRecs = [{ "name": "Aom1" }];
   commCompareResults( rc, expRecs );
   var rc = db.exec( "select * from " + csName + "." + clName + " where name like 'C+' " );
   var expRecs = [{ "name": "CCC6" }];
   commCompareResults( rc, expRecs );
   var rc = db.exec( "select * from " + csName + "." + clName + " where name like 'Bo*' " );
   var expRecs = [{ "name": "Bom4" }];
   commCompareResults( rc, expRecs );
   var rc = db.exec( "select * from " + csName + "." + clName + " where name like '[A-B]' " );
   var expRecs = [{ "name": "Aom1" }, { "name": "Aom2" }, { "name": "Bom4" }];
   commCompareResults( rc, expRecs );
   var rc = db.exec( "select * from " + csName + "." + clName + " where name like '.*m1{1,}' " );
   var expRecs = [{ "name": "Aom1" }];
   commCompareResults( rc, expRecs );
   var rc = db.exec( "select * from " + csName + "." + clName + " where name like 'Aom.' " );
   var expRecs = [{ "name": "Aom1" }, { "name": "Aom2" }];
   commCompareResults( rc, expRecs );
   var rc = db.exec( "select * from " + csName + "." + clName + " where name like 'B|T' " );
   var expRecs = [{ "name": "Bom4" }, { "name": "Tom5" }];
   commCompareResults( rc, expRecs );
   var rc = db.exec( "select * from " + csName + "." + clName + " where name like '^[aB]' " );
   var expRecs = [{ "name": "aom3" }, { "name": "Bom4" }];
   commCompareResults( rc, expRecs );
   var rc = db.exec( "select * from " + csName + "." + clName + " where name like '^[^aAB]' " );
   var expRecs = [{ "name": "Tom5" }, { "name": "CCC6" }, { "name": "D 7" }];
   commCompareResults( rc, expRecs );

   db.execUpdate( "delete from " + csName + "." + clName );
}
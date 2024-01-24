/******************************************************************************
@Description seqDB-7407: 内置SQL语法中的语句测试
                        1. create collectionspace
                        2. list collectionspaces
                        3. create collection
                        4. list collections
                        5. insert into
                        6. update 不带条件、带条件更新单条、带条件更新多条
                        7. create index 单字段索引、唯一索引、组合索引
                        8. drop index 单字段索引、唯一索引、组合索引
                        9. delete 带条件删除、不带条件删除
                        10. drop collection
                        11. drop collectionspace
@author liyuanyue
@date 2020-4-2
******************************************************************************/
main( test );

function test ()
{
   var csName = COMMCSNAME + "_7407";
   var clName = COMMCLNAME + "_7407";
   var commIndexName = CHANGEDPREFIX + "_7407_com_ix";
   var uniqIndexName = CHANGEDPREFIX + "_7407_uni_ix";
   var compIndexName = CHANGEDPREFIX + "_7407_cmp_ix";

   commDropCS( db, csName );

   // create collectionspace
   db.execUpdate( "create collectionspace " + csName );

   // list collectionspaces
   var rc = db.exec( "list collectionspaces" );
   compareResultByList( rc, csName, 1, " create collectionspace " );

   // create collection
   db.execUpdate( "create collection " + csName + "." + clName );

   // list collections
   var rc = db.exec( "list collections" );
   compareResultByList( rc, ( csName + "." + clName ), 1, " create collection " );

   // insert into 普通插入
   db.execUpdate( "insert into " + csName + "." + clName + " (name,age) values (\"Tom\",1)" );
   db.execUpdate( "insert into " + csName + "." + clName + " (name,age) values (\"Mike\",2)" );
   db.execUpdate( "insert into " + csName + "." + clName + " (name,age) values (\"Lisa\",3)" );
   db.execUpdate( "insert into " + csName + "." + clName + " (name,age) values (\"Json\",4)" );
   db.execUpdate( "insert into " + csName + "." + clName + " (name,age) values (\"Jhon\",5)" );
   db.execUpdate( "insert into " + csName + "." + clName + " (name,age) values (\"Tina\",6)" );
   db.execUpdate( "insert into " + csName + "." + clName + " (name,age) values (\"Pite\",7)" );

   var rc = db.exec( "select * from " + csName + "." + clName );
   var expRecs = [
      { "name": "Tom", "age": 1 }, { "name": "Mike", "age": 2 },
      { "name": "Lisa", "age": 3 }, { "name": "Json", "age": 4 },
      { "name": "Jhon", "age": 5 }, { "name": "Tina", "age": 6 },
      { "name": "Pite", "age": 7 }];
   commCompareResults( rc, expRecs );

   // update 不带条件、带条件更新单条、带条件更新多条
   db.execUpdate( "update " + csName + "." + clName + " set phone=123" );
   var rc = db.exec( "select * from " + csName + "." + clName );
   var expRecs = [
      { "name": "Tom", "age": 1, "phone": 123 }, { "name": "Mike", "age": 2, "phone": 123 },
      { "name": "Lisa", "age": 3, "phone": 123 }, { "name": "Json", "age": 4, "phone": 123 },
      { "name": "Jhon", "age": 5, "phone": 123 }, { "name": "Tina", "age": 6, "phone": 123 },
      { "name": "Pite", "age": 7, "phone": 123 }];
   commCompareResults( rc, expRecs );

   db.execUpdate( "update " + csName + "." + clName + " set age=10 where name=\"Lisa\"" );
   var rc = db.exec( "select * from " + csName + "." + clName + " where age =10" );
   var expRecs = [{ "name": "Lisa", "age": 10, "phone": 123 }];
   commCompareResults( rc, expRecs );

   db.execUpdate( "update " + csName + "." + clName + " set phone=456 where age>4" );
   var rc = db.exec( "select * from " + csName + "." + clName + " where age>4" );
   var expRecs = [
      { "name": "Lisa", "age": 10, "phone": 456 }, { "name": "Jhon", "age": 5, "phone": 456 },
      { "name": "Tina", "age": 6, "phone": 456 }, { "name": "Pite", "age": 7, "phone": 456 }];
   commCompareResults( rc, expRecs );

   var tmpclName = COMMCLNAME + "_7407_tmp";
   commDropCL( db, csName, tmpclName );
   var tmpcl = commCreateCL( db, csName, tmpclName, {}, false );

   // create index 单字段索引、唯一索引、组合索引
   db.execUpdate( "create index " + commIndexName + " on " + csName + "." + tmpclName + " (age desc)" );
   db.execUpdate( "create unique index " + uniqIndexName + " on " + csName + "." + tmpclName + " (age)" );
   db.execUpdate( "create index " + compIndexName + " on " + csName + "." + tmpclName + " (age,name)" );

   var rc = tmpcl.listIndexes();
   var expRecs = { commIndexName: commIndexName, uniqIndexName: uniqIndexName, compIndexName: compIndexName };
   var actRecs = {};
   while( rc.next() )
   {
      var obj = rc.current().toObj();
      var idxName = obj.IndexDef.name;
      if( idxName === commIndexName )
      {
         actRecs.commIndexName = commIndexName;
      }
      else if( idxName === uniqIndexName )
      {
         actRecs.uniqIndexName = uniqIndexName;
      }
      else if( idxName === compIndexName )
      {
         actRecs.compIndexName = compIndexName;
      }
   }
   assert.equal( expRecs, actRecs );

   // drop index 单字段索引、唯一索引、组合索引
   db.execUpdate( "drop index " + compIndexName + " on " + csName + "." + tmpclName );
   db.execUpdate( "drop index " + uniqIndexName + " on " + csName + "." + tmpclName );
   db.execUpdate( "drop index " + commIndexName + " on " + csName + "." + tmpclName );

   // 验证索引删除，只剩下$id索引
   var rc = tmpcl.listIndexes();
   var expCount = 1;
   var actCount = 0;
   while( rc.next() )
   {
      actCount++;
   }
   if( expCount !== actCount )
   {
      throw new Error( "check drop index error,expect result is " + expCount + ",but actually result is " + actRecs );
   }

   commDropCL( db, csName, tmpclName );

   // delete 带条件删除、不带条件删除
   db.execUpdate( "delete from " + csName + "." + clName + " where name=\"Lisa\"" );
   var rc = db.exec( "select * from " + csName + "." + clName );
   var expRecs = [
      { "name": "Tom", "age": 1, "phone": 123 }, { "name": "Mike", "age": 2, "phone": 123 },
      { "name": "Json", "age": 4, "phone": 123 }, { "name": "Jhon", "age": 5, "phone": 456 },
      { "name": "Tina", "age": 6, "phone": 456 }, { "name": "Pite", "age": 7, "phone": 456 }];
   commCompareResults( rc, expRecs );

   db.execUpdate( "delete from " + csName + "." + clName );
   var rc = db.exec( "select * from " + csName + "." + clName );
   var actResult = rc.size();
   if( actResult != 0 )
   {
      throw new Error( "Failed to check results. expected result 0,but actually result " + actResult );
   }

   // drop collection
   db.execUpdate( "drop collection " + csName + "." + clName );
   var rc = db.exec( "list collections" );
   compareResultByList( rc, ( csName + "." + clName ), 0, " drop collection " );

   // drop collectionspace
   db.execUpdate( "drop collectionspace " + csName );
   var rc = db.exec( "list collectionspaces" );
   compareResultByList( rc, csName, 0, " drop collectionspace " );
}

function compareResultByList ( rc, compareString, expResult, message )
{
   var number = 0;
   while( rc.next() )
   {
      if( rc.current().toObj().Name === compareString )
      {
         number++;
         break;
      }
   }
   if( number != expResult )
   {
      throw new Error( "Failed to check results." + message + "expected result " + expResult + ",but actually result " + number );
   }
}
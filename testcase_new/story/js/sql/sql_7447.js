/******************************************************************************
@Description seqDB-7447:transaction
@author liyuanyue
@date 2020-4-7
******************************************************************************/
testConf.clName = COMMCLNAME + "_7447";

main( test );

function test ()
{
   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_7447";

   for( var i = 0; i < 20; i++ )
   {
      db.execUpdate( "insert into " + csName + "." + clName + " (name,age) values (\"Tom\"," + i + ")" );
   }

   db.execUpdate( "begin transaction" );
   db.execUpdate( "insert into " + csName + "." + clName + " (name,age) values (\"Jack\",99)" );
   db.execUpdate( "update " + csName + "." + clName + " set age=99 where age=19" );
   db.execUpdate( "commit" );

   var rc = db.exec( "select * from " + csName + "." + clName + " where age=99" );
   var expRecs = [{ "name": "Tom", "age": 99 }, { "name": "Jack", "age": 99 }];
   commCompareResults( rc, expRecs );

   db.execUpdate( "begin transaction" );
   db.execUpdate( "delete from " + csName + "." + clName + " where age=99" );
   db.execUpdate( "rollback" );

   var rc = db.exec( "select * from " + csName + "." + clName + " where age=99" );
   var expRecs = [{ "name": "Tom", "age": 99 }, { "name": "Jack", "age": 99 }];
   commCompareResults( rc, expRecs );

   db.execUpdate( "delete from " + csName + "." + clName );
}
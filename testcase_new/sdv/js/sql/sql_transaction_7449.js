/****************************************************
@description:	db.transaction, then insert/update by SQL, basic case
         testlink cases:   seqDB-7449
@input:        1 insert into records
               2 exec [db.transBegin()]
               3 update by SQL
               4 select by SQL
               5 exec [db.transCommit()]
               6 exec [db.transRollback()], errorno: -195
               7 select by SQL
@output:    return success, and results correct.
@modify list:
      2015-5-13 ShanShan Hu added  2016-3-16 XiaoNi Huang modify
****************************************************/
csName = COMMCSNAME;
clName = CHANGEDPREFIX + "_bar";

function main ( db )
{
   println( "------Begin to ready cl." );
   try
   {
      // db.execUpdate("create collection "+csName+"."+clName);
      commDropCL( db, csName, clName, true, true, "drop cl in begin" );
      var opt = { ReplSize: 0 };
      var varCL = commCreateCL( db, csName, clName, opt, true, false, "create cl in begin" );
   }
   catch( e )
   {
      println( "Failed to drop/create cl in the begin." );
      throw e;
   }

   println( "------Begin to insert into records." );
   for( var i = 0; i < 20; i++ )
   {
      try
      {
         db.execUpdate( "insert into " + csName + "." + clName + "(age) values(" + i + ")" );
      }
      catch( e )
      {
         println( "Failed to insert records." );
         throw e;
      }
   }

   println( "------Begin to exec [db.transBegin()]." );
   try
   {
      db.transBegin();
   }
   catch( e )
   {
      println( "Failed to exec [db.transBegin()]." );
      throw e;
   }

   println( "------Begin to exec [update] by SQL.]" );
   try
   {
      db.execUpdate( "update " + csName + "." + clName + " set name=\"Tom\" where age=19" );
   }
   catch( e )
   {
      println( "Failed to update the records by SQL." );
      throw e;
   }

   println( "------Begin to exec [select] by SQL.]" );
   var rc;
   try
   {
      rc = db.exec( "select * from " + csName + "." + clName + " where name=\"Tom\"" );
   }
   catch( e )
   {
      println( "Failed to select the records by SQL." );
      throw e;
   }

   println( "------Begin to check results.]" );
   if( 1 != rc.size() )
   {
      throw "Failed to check results.";
   }

   println( "------Begin to exec [db.transCommit()]." );
   try
   {
      db.transCommit();
   }
   catch( e )
   {
      println( "Failed to exec [db.transCommit()]." );
      throw e;
   }

   println( "------Begin to exec [db.transRollback()]." );
   try
   {
      db.transRollback();
   }
   catch( e )
   {
      println( "Failed to exec [db.transRollback()]." );
      throw e;
   }

   println( "------Begin to exec [select] by SQL.]" );
   var rc;
   try
   {
      rc = db.exec( "select * from " + csName + "." + clName + " where name=\"Tom\"" );
   }
   catch( e )
   {
      println( "Failed to select the records." );
      throw e;
   }

   println( "------Begin to check results.]" );
   if( 1 != rc.size() )
   {
      println( " update operation execute successfull after the transcation commit" );
      throw -1;
   }

   println( "------Begin to drop cl in the end." );
   try
   {
      db.execUpdate( "drop collection " + csName + "." + clName );
   }
   catch( e )
   {
      println( "Failed to drop cl in the end." );
      throw e;
   }
}

main( db );
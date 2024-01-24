/****************************************************
@description:	db.transaction, then insert/update by SQL, basic case
         testlink cases:   seqDB-7450
@input:        1 insert into records
               2 exec [db.transBegin()]
               3 update by SQL
               4 select by SQL, and check results
               5 exec [db.close()], waiting auto rollback
               6 select by SQL, and check results
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
   sqlInsertAndCheck( db, csName, clName, 20, true, true,
      "Insert 20 records" );
   var count = varCL.count();
   println( "Query the " + varCL + "number : " + count );
   println( "CsName : " + csName + "ClName : " + clName );

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
      db.execUpdate( " update " + csName + "." + clName +
         " set name=\"Tom\" where age=10" );
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
      rc = db.exec( "select * from " + csName + "." + clName +
         " where name=\"Tom\"" );
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

   println( "------Begin to exec [db.close()]." );
   try
   {
      db.close();
   }
   catch( e )
   {
      println( "Failed to disconnect." );
      throw e;
   }

   // Whether the update operation is successful after interruption
   var maxRollbackTime = 10;
   var timeCount = 0;

   var rc;
   var size = 0;

   println( "------Begin to exec [new Sdb]." );
   try
   {
      db = new Sdb( COORDHOSTNAME, COORDSVCNAME );
   }
   catch( e )
   {
      println( "Failed to connect." );
      throw e;
   }

   println( "------Begin to exec [select] by SQL.]" );
   while( true )
   {
      try
      {
         rc = db.exec( "select * from " + csName + "." + clName +
            " where name=\"Tom\"" );
         size = rc.size();
         println( "select data size: " + size );
      }
      catch( e )
      {
         println( "Failed to select by SQL." );
         throw e;
      }

      println( "------Begin to check results.]" );
      if( 0 != size )
      {
         if( timeCount < maxRollbackTime )
         {
            sleep( 1000 );
            timeCount++;
            continue;
         }
         println( "Roll back failed or time out after connect close, size: " + size );
         throw "Roll back failed or time out";
      }
      else
      {
         break;
      }
   }

   var count = 0;
   // Clear environment
   while( true )
   {
      ++count;
      try
      {
         db.execUpdate( "drop collection " + csName + "." + clName );
         break;
      }
      catch( e )
      {
         if( e == -190 && count <= 10 )
         {
            sleep( 1000 );
            // wait the rollback end for some seconds
         }
         else
         {
            println( "unexpected err happened when clear cl:" + e );
            throw e;
         }
      }
   }
}

main( db );

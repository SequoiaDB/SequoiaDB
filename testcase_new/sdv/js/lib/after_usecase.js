/* *****************************************************************************
@discretion: Clean after test-case
@modify list:
   2014-2-28 Jianhui Xu  Init
***************************************************************************** */

// RUNRESULT is input parameter
if( typeof ( RUNRESULT ) == "undefined" )
{
   RUNRESULT = 0;
}
var db = new Sdb( COORDHOSTNAME, COORDSVCNAME );

function main ( db )
{
   // 1. check nodes
   var groups = commGetGroups( db, "", "", false );
   var checkLSN = false;
   if( 0 != RUNRESULT )
   {
      checkLSN = true;
   }
   var errNodes = commCheckBusiness( groups, checkLSN );
   if( errNodes.length == 0 )
   {
   }
   else
   {
      println( "Groups or nodes has error after test-case: " );
      commPrint( errNodes );
   }

   if( 0 != RUNRESULT )
   {
      // not clean
      return;
   }

   // 2. drop CHANGEDPREFIX's all collection
   var cols = commGetCSCL( db, CHANGEDPREFIX );
   for( var i = 0; i < cols.length; ++i )
   {
      for( var j = 0; j < cols[i].cl.length; ++j )
      {
         // can't drop dummy collection
         if( COMMDUMMYCLNAME == cols[i].cl[j] )
         {
            continue;
         }
         try
         {
            var str = cols[i].cl[j];
            var clname = str.split( "." )[1];
            commDropCL( db, cols[i].cs, clname, true, true, "After test-case" );
         }
         catch( e )
         {
            println( "Drop " + cols[i].cs + "." + clname + " failed after test-case: " + e );
         }
      }
   }

   // 3. drop CHANGEDPREFIX backup
   var backups = commGetBackups( db, CHANGEDPREFIX );
   for( var j = 0; j < backups.length; ++j )
   {
      try
      {
         db.removeBackup( { "Name": backups[j] } );
      }
      catch( e )
      {
         println( "Drop backup " + backups[j] + " failed after test-case: " + e );
      }
   }

   // 4. drop CHANGEDPREFIX domain
   var domains = commGetDomains( db, CHANGEDPREFIX );
   for( var j = 0; j < domains.length; ++j )
   {
      try
      {
         db.dropDomain( domains[j] );
      }
      catch( e )
      {
         println( "Drop domain " + domains[j] + " failed after test-case: " + e );
      }
   }

   // 5. drop CHANGEDPREFIX procedure
   var procedures = commGetProcedures( db, CHANGEDPREFIX );
   for( var j = 0; j < procedures.length; ++j )
   {
      try
      {
         db.removeProcedure( procedures[j] );
      }
      catch( e )
      {
         println( "Drop procedure " + procedures[j] + " failed after test-case: " + e );
      }
   }
}

try
{
   main( db );
}
catch( e )
{
   println( "After test-case environment clear failed: " + e );
}


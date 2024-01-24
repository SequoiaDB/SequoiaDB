/* *****************************************************************************
@discretion: Prepare before test-case
@modify list:
   2014-2-28 Jianhui Xu  Init
***************************************************************************** */

var db = new Sdb( COORDHOSTNAME, COORDSVCNAME );

function main ( db )
{
   // 1. check nodes
   var groups = commGetGroups( db, "", "", false );
   var errNodes = commCheckBusiness( groups );
   if( errNodes.length == 0 )
   {
   }
   else
   {
      println( "Has " + errNodes.length + " nodes in fault before test-case: " );
      commPrint( errNodes );
   }

   // 2. create COMMCSNAME
   commCreateCS( db, COMMCSNAME, true, "before test-case" );

   // 3. drop CHANGEDPREFIX's all collection
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
            commDropCL( db, cols[i].cs, clname, true, true, "before test-case" );
         }
         catch( e )
         {
            println( "Drop " + cols[i].cs + "." + clname + " failed before test-case: " + e );
         }
      }
   }

   // 4. drop CHANGEDPREFIX backup
   var backups = commGetBackups( db, CHANGEDPREFIX );
   for( var j = 0; j < backups.length; ++j )
   {
      try
      {
         db.removeBackup( { "Name": backups[j] } );
      }
      catch( e )
      {
         println( "Drop backup " + backups[j] + " failed before test-case: " + e );
      }
   }

   // 5. drop CHANGEDPREFIX domain
   var domains = commGetDomains( db, CHANGEDPREFIX );
   for( var j = 0; j < domains.length; ++j )
   {
      try
      {
         db.dropDomain( domains[j] );
      }
      catch( e )
      {
         println( "Drop domain " + domains[j] + " failed before test-case: " + e );
      }
   }

   // 6. drop CHANGEDPREFIX procedure
   var procedures = commGetProcedures( db, CHANGEDPREFIX );
   for( var j = 0; j < procedures.length; ++j )
   {
      try
      {
         db.removeProcedure( procedures[j] );
      }
      catch( e )
      {
         println( "Drop procedure " + procedures[j] + " failed before test-case: " + e );
      }
   }
}

try
{
   main( db );
}
catch( e )
{
   println( "Before test-case environment prepare failed: " + e );
}


/* *****************************************************************************
@discretion: Clean after all test-cases
@modify list:
   2014-3-1 Jianhui Xu  Init
***************************************************************************** */

// RUNRESULT is input parameter
if( typeof ( RUNRESULT ) == "undefined" )
{
   RUNRESULT = 0;
}
var db = new Sdb( COORDHOSTNAME, COORDSVCNAME );

function main ( db )
{
   sleep( 1000 );
   // 1. check nodes
   var groups = commGetGroups( db, "", "", false );
   var errNodes = commCheckBusiness( groups, true );
   if( errNodes.length == 0 )
   {
   }
   else
   {
      println( "Has " + errNodes.length + " nodes in fault after all test-cases: " );
      commPrint( errNodes );
   }

   if( 0 != RUNRESULT )
   {
      // not clean
      return;
   }

   // 2. drop CHANGEDPREFIX's all collection space
   var cols = commGetCSCL( db, CHANGEDPREFIX );
   for( var i = 0; i < cols.length; ++i )
   {
      try
      {
         commDropCS( db, cols[i].cs, true, " after all test-cases" );
      }
      catch( e )
      {
         println( "Drop " + cols[i].cs + " failed after all test-cases: " + e );
      }
   }

}

try
{
   main( db );
}
catch( e )
{
   println( "After all test-cases environment clean failed: " + e );
}

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
   // 1. 删除名称含 local_test 的 cs
   var cols = commGetSnapshot( db, SDB_SNAP_COLLECTIONSPACES, { "Name": Regex( CHANGEDPREFIX, "i" ) }, { "Name": "" } );
   for( var i = 0; i < cols.length; ++i )
   {
      commDropCS( db, cols[i].Name, true, " before all test-cases" );
   }
}

try
{
   if( !RUNRESULT ) 
   {
      main( db );
   }
}
catch( e )
{
   println( "After all test-cases environment clean failed: " + e );
}

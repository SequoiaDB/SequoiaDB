var db = new Sdb( COORDHOSTNAME, COORDSVCNAME );
function main ( db )
{
   //check cluster
   var groups = commGetGroups( db, "", "", false );
   //check group num
   var dataGroups = commGetGroups( db, false, "", true, true, true );
   if( dataGroups.length == 0 )
   {
      throw "No group found";
   }
   else if( dataGroups.length != 3 ) 
   {
      throw "groups not 3";
   }
   else 
   {
      //check node 
      var groups = commGetGroups( db, "", "", false );
      var errNodes = commCheckBusiness( groups, true );
      if( errNodes.length == 0 )
      {
      }
      else
      {
         println( "Has " + errNodes.length + " nodes in fault before all test-cases: " );
         println( JSON.stringify( errNodes, "", 3 ) );
      }
   }

}

try
{
   main( db );
}
catch( e )
{
   println( "Before detect cluster failed: " + e );
}
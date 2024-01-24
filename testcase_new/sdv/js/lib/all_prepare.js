/* *****************************************************************************
@discretion: Prepare before all test-case
@modify list:
   2014-3-1 Jianhui Xu  Init
***************************************************************************** */

var db = new Sdb( COORDHOSTNAME, COORDSVCNAME );

function createDummyCollection ( db )
{
   if( commIsStandalone( db ) )
   {
      commCreateCL( db, COMMCSNAME, COMMDUMMYCLNAME, {}, true, true, "Create dummy collection" );
   }
   else
   {
      var dataGroups = commGetGroups( db, false, "", true, true, true );
      if( dataGroups.length == 0 )
      {
         throw "No group found";
      }
      var sourceGroup = dataGroups[0][0]["GroupName"];
      var cl = commCreateCL( db, COMMCSNAME, COMMDUMMYCLNAME,
         { Group: sourceGroup, ShardingKey: { a: 1 }, ShardingType: 'hash', Partition: 4096 },
         true, true, "Create dummy collection" );
      for( var i = 1; i < dataGroups.length; ++i )
      {
         cl.split( sourceGroup, dataGroups[i][0]["GroupName"], 1 );
      }
   }
}

function main ( db )
{
   // 1. check nodes
   var groups = commGetGroups( db, "", "", false );
   var errNodes = commCheckBusiness( groups, true );
   if( errNodes.length == 0 )
   {
   }
   else
   {
      println( "Has " + errNodes.length + " nodes in fault before all test-cases: " );
      commPrint( errNodes );
   }

   // 2. drop CHANGEDPREFIX's all collection space
   var cols = commGetCSCL( db, CHANGEDPREFIX );
   for( var i = 0; i < cols.length; ++i )
   {
      try
      {
         commDropCS( db, cols[i].cs, true, " before all test-cases" );
      }
      catch( e )
      {
         println( "Drop " + cols[i].cs + " failed before all test-cases: " + e );
      }
   }

   // 3. create dummy collection and split to all group
   createDummyCollection( db );

   commMakeDir( COORDHOSTNAME, WORKDIR );
}

try
{
   main( db );
}
catch( e )
{
   println( "Before all test-cases environment prepare failed: " + e );
}

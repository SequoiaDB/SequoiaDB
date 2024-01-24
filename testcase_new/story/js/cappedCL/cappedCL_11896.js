/************************************
*@Description: snapshot collectionspaces, collections, storages
*@author:      liuxiaoxuan
*@createdate:  2019.7.17
*@testlinkCase: seqDB-11896
**************************************/


main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var csName1 = COMMCAPPEDCSNAME + "_11896_1";
   var csName2 = COMMCAPPEDCSNAME + "_11896_2";
   commDropCS( db, csName1, true, "drop CS in the beginning" );
   commDropCS( db, csName2, true, "drop CS in the beginning" );
   commCreateCS( db, csName1, true, "", { Capped: true } );
   commCreateCS( db, csName2, true, "", { Capped: true } );

   var clOption = { Capped: true, Size: 1024, AutoIndexId: false };
   var clName = COMMCAPPEDCLNAME + "_11896";
   commCreateCL( db, csName1, clName, clOption, true, true );
   commCreateCL( db, csName2, clName, clOption, true, true );

   // connect coord, check snapshot 5
   var expectResult = [csName1, csName2];
   checkSnapshotResult( db, expectResult, SDB_SNAP_COLLECTIONSPACES );

   // connect data, check snapshot 5
   var groupNames = commGetCLGroups( db, csName1 + "." + clName );
   var masterNode = db.getRG( groupNames[0] ).getMaster();
   var masterDB = new Sdb( masterNode.getHostName(), masterNode.getServiceName() );
   expectResult = [csName1];
   checkSnapshotResult( masterDB, expectResult, SDB_SNAP_COLLECTIONSPACES );

   // connect coord, check snapshot 4
   expectResult = [csName1 + "." + clName, csName2 + "." + clName];
   checkSnapshotResult( db, expectResult, SDB_SNAP_COLLECTIONS );

   // connect data, check snapshot 4
   expectResult = [csName1 + "." + clName];
   checkSnapshotResult( masterDB, expectResult, SDB_SNAP_COLLECTIONS );
   masterDB.close();

   commDropCS( db, csName1, true, "drop CS in the end" );
   commDropCS( db, csName2, true, "drop CS in the end" );
}

function checkSnapshotResult ( db, expectResult, snapshotType )
{
   var actResult = [];
   var cur = db.snapshot( snapshotType ).toArray();
   for( var i = 0; i < cur.length; i++ )
   {
      var rec = eval( "(" + cur[i] + ")" );
      actResult.push( rec["Name"] );
   }

   for( var i = 0; i < expectResult.length; i++ )
   {
      if( actResult.indexOf( expectResult[i] ) === -1 )
      {
         throw new Error( "check snapshot check snapshot" + expectResult + actResult );
      }
   }
}
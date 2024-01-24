/************************************
*@Description: list collectionspaces, collections, storages
*@author:      liuxiaoxuan
*@createdate:  2019.7.17
*@testlinkCase: seqDB-11894/seqDB-11895
**************************************/


main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }
   var csName1 = COMMCAPPEDCSNAME + "_11894_11895_1";
   var csName2 = COMMCAPPEDCSNAME + "_11894_11895_2";
   commDropCS( db, csName1, true, "drop CS in the beginning" );
   commDropCS( db, csName2, true, "drop CS in the beginning" );
   commCreateCS( db, csName1, true, "", { Capped: true } );
   commCreateCS( db, csName2, true, "", { Capped: true } );

   var clOption = { Capped: true, Size: 1024, AutoIndexId: false };
   var clName = COMMCAPPEDCLNAME + "_11894_11895";
   commCreateCL( db, csName1, clName, clOption, true, true );
   commCreateCL( db, csName2, clName, clOption, true, true );

   // check list 5
   var expectResult = [csName1, csName2];
   checkListResult( db, expectResult, SDB_LIST_COLLECTIONSPACES );

   // check list 4
   expectResult = [csName1 + "." + clName, csName2 + "." + clName];
   checkListResult( db, expectResult, SDB_LIST_COLLECTIONS );

   // check list 6
   var groupNames = commGetCLGroups( db, csName1 + "." + clName );
   var masterNode = db.getRG( groupNames[0] ).getMaster();
   var masterDB = new Sdb( masterNode.getHostName(), masterNode.getServiceName() );
   expectResult = [csName1];
   checkListResult( masterDB, expectResult, SDB_LIST_STORAGEUNITS );
   masterDB.close();

   commDropCS( db, csName1, true, "drop CS in the end" );
   commDropCS( db, csName2, true, "drop CS in the end" );
}

function checkListResult ( db, expectResult, listType )
{
   var actResult = [];
   var cur = db.list( listType ).toArray();
   for( var i = 0; i < cur.length; i++ )
   {
      var rec = eval( "(" + cur[i] + ")" );
      actResult.push( rec["Name"] );
   }

   for( var i = 0; i < expectResult.length; i++ )
   {
      if( actResult.indexOf( expectResult[i] ) === -1 )
      {
         throw new Error( "check list check list" + expectResult + actResult );
      }
   }
}
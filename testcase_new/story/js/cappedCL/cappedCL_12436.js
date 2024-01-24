/************************************
*@Description: 系统重建固定集合时，属性正确
*@author:      liuxiaoxuan
*@createdate:  2019.7.17
*@testlinkCase: seqDB-12436
**************************************/


main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clOption = { Capped: true, Size: 1, AutoIndexId: false };
   var clName = COMMCAPPEDCLNAME + "12436";
   var dbcl = commCreateCL( db, COMMCAPPEDCSNAME, clName, clOption, false, true );

   // connect master node, delete capped cl
   var groupNames = commGetCLGroups( db, COMMCAPPEDCSNAME + "." + clName );
   var masterNode = db.getRG( groupNames[0] ).getMaster();
   var masterDB = new Sdb( masterNode.getHostName(), masterNode.getServiceName() );
   masterDB.getCS( COMMCAPPEDCSNAME ).dropCL( clName );

   // check masterDB not exist capped cl
   var expectCappedCLExist = true;
   var expectResult = [COMMCAPPEDCSNAME + "." + clName];
   checkSnapshotResult( masterDB, expectResult, SDB_SNAP_COLLECTIONS, !expectCappedCLExist );

   // connect coord to insert
   var recordNum = 100;
   var stringLength = 10;
   var fieldString = "test";
   insertFixedLengthDatas( dbcl, recordNum, stringLength, fieldString );

   // connect master node, check snapshot 4, cappedcl rebuild
   checkSnapshotResult( masterDB, expectResult, SDB_SNAP_COLLECTIONS, expectCappedCLExist );

   masterDB.close();
   commDropCL( db, COMMCAPPEDCSNAME, clName, true, true, "drop CL in the end" );
}

function checkSnapshotResult ( db, expectResult, snapshotType, expectExist )
{
   // default expect "expectResult" exist in snapshot
   if( typeof ( expectExist ) == "undefined" ) 
   {
      expectExist = true;
   }

   var actResult = [];
   var cur = db.snapshot( snapshotType ).toArray();
   for( var i = 0; i < cur.length; i++ )
   {
      var rec = eval( "(" + cur[i] + ")" );
      actResult.push( rec["Name"] );
   }

   for( var i = 0; i < expectResult.length; i++ )
   {
      // expect "expectResult" exist in snapshot
      if( expectExist && actResult.indexOf( expectResult[i] ) === -1 )
      {
         throw new Error( "check snapshot check snapshot" + expectResult + actResult );
      }
      // expect "expectResult" not exist in snapshot
      else if( !expectExist && actResult.indexOf( expectResult[i] ) !== -1 )
      {
         throw new Error( "check snapshot check snapshot" + expectResult + actResult );
      }
   }

}
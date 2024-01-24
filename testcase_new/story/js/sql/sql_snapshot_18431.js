/************************************
*@Description: 内置 SQL 中 $SNAPSHOT_CONFIGS 使用 with_option
*@author:      chensiqin
*@createdate:  2019.06.13
*@testlinkCase: seqDB-18431
**************************************/

main( test );
function test ()
{
   if( commGetGroupsNum( db ) < 1 )
   {
      return;
   }
   var groupNames = getGroup( db );

   var option = new SdbSnapshotOption().cond( { GroupName: groupNames[0] } ).sort( { NodeName: 1 } ).options( { "Mode": "run", "Expand": false } );
   var expectedInfo = db.snapshot( SDB_SNAP_CONFIGS, option );
   var actualInfo = db.exec( 'select * from $SNAPSHOT_CONFIGS where GroupName = "' + groupNames[0] + '" order by NodeName asc /*+use_option(Mode, run) use_option(Expand, false)*/' );
   checkRec( actualInfo, expectedInfo );

   var option = new SdbSnapshotOption().cond( { GroupName: groupNames[0] } ).sort( { NodeName: 1 } ).options( { "Mode": "local", "Expand": false } );
   var expectedInfo = db.snapshot( SDB_SNAP_CONFIGS, option );
   var actualInfo = db.exec( 'select * from $SNAPSHOT_CONFIGS where GroupName = "' + groupNames[0] + '" order by NodeName asc /*+use_option(Mode, local) use_option(Expand, false)*/' );
   checkRec( actualInfo, expectedInfo );
}

function getGroup ( db )
{
   var listGroups = db.listReplicaGroups();
   var groupArray = new Array();
   while( listGroups.next() )
   {
      if( listGroups.current().toObj()["GroupID"] >= DATA_GROUP_ID_BEGIN )
      {
         groupArray.push( listGroups.current().toObj()["GroupName"] );
      }
   }
   return groupArray;
}

function checkRec ( actualRc, expectedRc )
{
   //get actual records to array
   var actRecs = [];
   var expRecs = [];

   while( actualRc.next() )
   {
      actRecs.push( actualRc.current().toObj() );
   }

   while( expectedRc.next() )
   {
      expRecs.push( expectedRc.current().toObj() );
   }

   //check count
   assert.equal( actRecs.length, expRecs.length );

   //check every records every fields
   for( var i in expRecs )
   {
      var actRec = actRecs[i];
      var expRec = expRecs[i];
      for( var f in expRec )
      {
         assert.equal( actRec[f], expRec[f] );
      }
   }

}
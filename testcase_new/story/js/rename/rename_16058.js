/* *****************************************************************************
@discretion: rename cl after split cl
@author��2018-10-12 wuyan  Init
***************************************************************************** */

main( test );
function test ()
{
   //@ clean before
   if( true == commIsStandalone( db ) )
   {
      return;
   }
   //less two groups no split
   var allGroupName = getGroupName( db, true );
   if( 1 === allGroupName.length )
   {
      return;
   }

   var clName = CHANGEDPREFIX + "_renamecl16058";
   var newCLName = CHANGEDPREFIX + "_newcl16058";
   commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the beginning" );
   commDropCL( db, COMMCSNAME, newCLName, true, true, "clear collection in the beginning" );
   var options = { ShardingKey: { no: 1 }, ShardingType: "range", ReplSize: 0, Compressed: true };
   var dbcl = commCreateCL( db, COMMCSNAME, clName, options, true, true );

   //insert records 
   var recordNums = 2000;
   insertData( dbcl, recordNums );

   var percent = 50;
   var targetGroupNums = 2;
   var groupsInfo = getSplitGroups( COMMCSNAME, clName, targetGroupNums );
   var srcGroupName = groupsInfo[0].GroupName;
   var dstGroupName = groupsInfo[1].GroupName;
   splitCL( dbcl, srcGroupName, dstGroupName );

   db.getCS( COMMCSNAME ).renameCL( clName, newCLName );
   var newcl = db.getCS( COMMCSNAME ).getCL( newCLName );

   checkRenameCLResult( COMMCSNAME, clName, newCLName );
   checkDatas( COMMCSNAME, newCLName, recordNums, groupsInfo );

   commDropCL( db, COMMCSNAME, newCLName, true, true, "clear collection in the ending" );

}

function checkDatas ( csName, newCLName, expRecordNums, groupsInfo )
{
   //check the record nums      
   var dbcl = db.getCS( csName ).getCL( newCLName );
   var count = dbcl.count();
   assert.equal( count, expRecordNums );

   //test record nums of split groups
   for( var i = 0; i < 2; i++ )
   {
      try
      {
         var sdb = new Sdb( groupsInfo[i].HostName, groupsInfo[i].svcname );
         var cl = sdb.getCS( csName ).getCL( newCLName );
         var num = cl.count();
         assert.equal( num, expRecordNums / 2 );

      } finally
      {
         if( sdb !== undefined )
         {
            sdb.close();
            sdb = undefined;
         }
      }
   }
}

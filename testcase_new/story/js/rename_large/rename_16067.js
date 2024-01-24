/* *****************************************************************************
@discretion: rename cl ,the cl is spliting
@author��2018-10-15 wuyan  Init
***************************************************************************** */

main( test );
function test ()
{
   if( commGetGroupsNum( db ) < 2 )
   {
      return;
   }

   var clName = CHANGEDPREFIX + "_renamecl16067";
   var newCLName = CHANGEDPREFIX + "_newcl16067";
   commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the beginning" );
   commDropCL( db, COMMCSNAME, newCLName, true, true, "clear collection in the beginning" );
   var options = { ShardingKey: { no: 1 }, ShardingType: "range", ReplSize: 0, Compressed: true };

   var dbcl = commCreateCL( db, COMMCSNAME, clName, options, true, true );
   var recordNums = 10000;
   insertData( dbcl, recordNums );

   var targetGroupNums = 2;
   var groupsInfo = getSplitGroups( COMMCSNAME, clName, targetGroupNums );
   var taskId = splitCL( COMMCSNAME, clName );
   assert.tryThrow( SDB_OPERATION_CONFLICT, function()
   {
      db.getCS( COMMCSNAME ).renameCL( clName, newCLName );
   } );

   checkSplitResult( COMMCSNAME, clName, taskId, recordNums, groupsInfo );
   checkRenameCLResult( COMMCSNAME, newCLName, clName );

   commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the ending" );
}

function splitCL ( csName, clName )
{
   var dbcl = db.getCS( csName ).getCL( clName );
   var percent = 50;
   var targetGroupNums = 2;
   var groupsInfo = getSplitGroups( COMMCSNAME, clName, targetGroupNums );
   var srcGroupName = groupsInfo[0].GroupName;
   var dstGroupName = groupsInfo[1].GroupName;
   var taskId = dbcl.splitAsync( srcGroupName, dstGroupName, percent );
   return taskId;
}

function checkSplitResult ( csName, clName, taskId, expRecordNums, groupsInfo )
{
   //waiting for split 
   var sleepInteval = 10;
   var sleepDuration = 0;
   var maxSleepDuration = 100000;

   while( ( db.listTasks( { "TaskID": taskId } ).next() !== undefined ) && sleepDuration < maxSleepDuration )
   {
      sleep( sleepInteval );
      sleepDuration += sleepInteval;
   }
   //check the record nums      
   var dbcl = db.getCS( csName ).getCL( clName );
   var count = dbcl.count();
   assert.equal( count, expRecordNums );

   //test record nums of split groups
   for( var i = 0; i < 2; i++ )
   {
      try
      {
         var sdb = new Sdb( groupsInfo[i].HostName, groupsInfo[i].svcname );
         var cl = sdb.getCS( csName ).getCL( clName );
         var num = cl.count();
         assert.equal( num, expRecordNums / 2 );
      } finally
      {
         if( sdb !== undefined )
         {
            sdb.close();
         }
      }
   }
}

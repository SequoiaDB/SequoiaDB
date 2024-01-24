/******************************************************************************
@description seqDB-16328:检测异步切分任务建立后返回的任务ID
@author 2018-11-06 wangkexin init; 2020-01-13 huangxiaoni modify
******************************************************************************/
main( test );

function test ()
{
   if( true == commIsStandalone( db ) )
   {
      return;
   }

   if( commGetGroupsNum( db ) < 2 )
   {
      return;
   }

   var groupNames = commGetDataGroupNames( db );
   var srcGroupName = groupNames[0];
   var dstGroupName = groupNames[1];
   var clName = CHANGEDPREFIX + "_split_16328";
   var recordNums = 30000;

   commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the beginning" );
   var options = { "ShardingKey": { "No": 1 }, "ShardingType": "range", "Group": srcGroupName };
   var cl = commCreateCL( db, COMMCSNAME, clName, options );

   insertData( cl, recordNums );
   var taskId = cl.splitAsync( srcGroupName, dstGroupName, 90 );
   // check taskId
   var tasks = db.listTasks( { "TaskID": taskId } ).toArray();
   if( tasks.length !== 1 )
   {
      throw new Error( "check taskId fail, exist the taskId " + taskId + ", but listTasks not exist the taskId." );
   }

   commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the ending" );
}
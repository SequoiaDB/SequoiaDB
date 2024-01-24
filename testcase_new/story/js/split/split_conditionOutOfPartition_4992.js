/************************************
*@description ： seqDB-4992:hash范围切分设置condition条件值超出partition范围
*@author ：2019-5-30 wangkexin init; 2020-01-13 huangxiaoni modify
**************************************/
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
   var clName = CHANGEDPREFIX + "_split_4992";

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning." );
   var options = { ShardingKey: { a: 1 }, ShardingType: "hash", Partition: 1024, Group: srcGroupName };
   var cl = commCreateCL( db, COMMCSNAME, clName, options );
   insertData( cl, 100 );

   var condition = { "Partition": 1024 };
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.split( srcGroupName, dstGroupName, condition );
   } );

   var condition = { "Partition": -1 };
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.split( srcGroupName, dstGroupName, condition );
   } );

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the end." );
}
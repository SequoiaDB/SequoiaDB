/************************************
*@description ：seqDB-4994:range分区组进行范围切分，其中condition字段值不在源分区组内
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
   var clName = CHANGEDPREFIX + "_split_4994";

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning." );
   var options = { ShardingKey: { a: 1 }, ShardingType: "range", Group: srcGroupName };
   var cl = commCreateCL( db, COMMCSNAME, clName, options );
   insertData( cl, 100 );

   //test a : condition为null
   var condition = null;
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.split( srcGroupName, dstGroupName, condition );
   } );

   //test b : condition为空串
   condition = '';
   assert.tryThrow( SDB_OUT_OF_BOUND, function()
   {
      cl.split( srcGroupName, dstGroupName, condition );
   } );

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the end." );
}
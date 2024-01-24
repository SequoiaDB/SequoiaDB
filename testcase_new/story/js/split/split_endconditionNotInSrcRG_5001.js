/************************************
*@description： seqDB-5001:range分区组进行范围切分，其中endcondition字段值不在源分区组内_ST.split.01.024
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
   var clName = CHANGEDPREFIX + "_split_5001";

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning." );
   var options = { ShardingKey: { a: 1 }, ShardingType: "range", Group: srcGroupName };
   var cl = commCreateCL( db, COMMCSNAME, clName, options );
   insertData( cl, 100 );

   //test a : endcondition 为null
   var endcondition = null;
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.split( srcGroupName, dstGroupName, 0, endcondition );
   } );

   //test b : endcondition 为空串
   endcondition = '';
   assert.tryThrow( SDB_OUT_OF_BOUND, function()
   {
      cl.split( srcGroupName, dstGroupName, endcondition );
   } );

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the end." );
}
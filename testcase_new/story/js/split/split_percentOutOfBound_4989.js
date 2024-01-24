/************************************
*@description ：seqDB-4989:percent超过边界值
*@author ：2019-5-30 wangkexin
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
   var csName = COMMCSNAME;
   var clName = CHANGEDPREFIX + "_split_4989";

   commDropCL( db, csName, clName, true, true, "drop CL in the beginning." );
   var options = { ShardingKey: { a: 1 }, ShardingType: "hash", Group: srcGroupName };
   var cl = commCreateCL( db, csName, clName, options );
   insertData( cl, 100 );

   checkPercentOutOfBound( cl, srcGroupName, dstGroupName, -0.001 );
   checkPercentOutOfBound( cl, srcGroupName, dstGroupName, 0 );
   checkPercentOutOfBound( cl, srcGroupName, dstGroupName, 100.001 );

   commDropCL( db, csName, clName, true, true, "drop CL in the end." );
}

function checkPercentOutOfBound ( cl, srcGroupName, dstGroupName, percent )
{
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.split( srcGroupName, dstGroupName, percent );
   } );
}
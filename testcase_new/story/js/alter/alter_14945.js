/************************************
*@Description: 分区表关闭切分功能后执行切分
*@author:      luweikang
*@createdate:  2018.4.25
*@testlinkCase:seqDB-14945
**************************************/

main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }
   //less two groups no split
   var allGroupName = getGroupName( db, true );
   if( 1 === allGroupName.length )
   {
      return;
   }

   var csName = COMMCSNAME;
   var clName = CHANGEDPREFIX + "_14945";

   var options = { ShardingType: 'hash', ShardingKey: { a: 1 } };
   var cl = commCreateCL( db, csName, clName, options, true, false, "create CL in the begin" );

   //关闭分区功能
   cl.disableSharding();

   var splitGroup = getSplitGroup( db, csName, clName );
   assert.tryThrow( SDB_COLLECTION_NOTSHARD, function()
   {
      cl.split( splitGroup.srcGroup, splitGroup.tarGroup, 50 );
   } );

   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingKey", undefined );
   commDropCL( db, csName, clName, true, false, "clean cl" );
}
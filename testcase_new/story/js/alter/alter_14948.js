/************************************
*@Description: 普通表修改ShardingKey
*@author:      luweikang
*@createdate:  2018.4.25
*@testlinkCase:seqDB-14948
**************************************/

main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var csName = COMMCSNAME;
   var clName = CHANGEDPREFIX + "_14948";

   var cl = commCreateCL( db, csName, clName, {}, true, false, "create CL in the begin" );

   //only alter ShardingType
   assert.tryThrow( SDB_NO_SHARDINGKEY, function()
   {
      cl.setAttributes( { ShardingType: 'range' } );
   } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingKey", undefined );

   //alter ShardingKey and ShardingType
   cl.setAttributes( { ShardingType: 'range', ShardingKey: { a: 1 } } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingKey", { a: 1 } );

   commDropCL( db, csName, clName, true, false, "clean cl" );
}
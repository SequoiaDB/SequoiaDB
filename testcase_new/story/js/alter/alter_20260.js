/******************************************************************************
*@Description: 分区表修改AutoSplit字段
*@author:      luweikang
*@createdate:  2019.11.15
*@testlinkCase:seqDB-20260
******************************************************************************/
main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }
   var groupName = commGetGroups( db );
   if( groupName.length < 2 )
   {
      return;
   }

   var clName = "alter20260";
   commDropCL( db, COMMCSNAME, clName );

   var cl = commCreateCL( db, COMMCSNAME, clName, { ShardingKey: { id: 1 }, ShardingType: "hash", AutoSplit: true } );

   //alters AutoSplit
   cl.alter( { AutoSplit: true } );
   checkSnapshot( db, SDB_SNAP_CATALOG, COMMCSNAME, clName, "AutoSplit", true );

   cl.setAttributes( { AutoSplit: true } );
   checkSnapshot( db, SDB_SNAP_CATALOG, COMMCSNAME, clName, "AutoSplit", true );

   assert.tryThrow( SDB_OPTION_NOT_SUPPORT, function()
   {
      cl.alter( { AutoSplit: false } );
   } );
   checkSnapshot( db, SDB_SNAP_CATALOG, COMMCSNAME, clName, "AutoSplit", true );

   assert.tryThrow( SDB_OPTION_NOT_SUPPORT, function()
   {
      cl.setAttributes( { AutoSplit: false } );
   } );
   checkSnapshot( db, SDB_SNAP_CATALOG, COMMCSNAME, clName, "AutoSplit", true );

   //clean test-env
   commDropCL( db, COMMCSNAME, clName );
}
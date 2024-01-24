/************************************
*@Description: alter批量修改cl属性
*@author:      luweikang
*@createdate:  2018.4.25
*@testlinkCase:seqDB-15218
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
   var clName = CHANGEDPREFIX + "_15218";

   var cl = commCreateCL( db, csName, clName, { "Compressed": false }, true, false, "create CL in the begin" );

   for( i = 0; i < 5000; i++ )
   {
      cl.insert( { a: i, b: "sequoiadb test split cl alter option" } );
   }

   cl.alter( {
      Alter: [{ Name: "enable sharding", Args: { ShardingKey: { a: 1 }, ShardingType: 'hash', AutoSplit: false } },
      { Name: "set attributes", Args: { Name: "cs.cl" } },
      { Name: "set attributes", Args: { StrictDataMode: true } }],
      Options: { IgnoreException: true }
   } )
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingKey", { a: 1 } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingType", "hash" );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "AutoSplit", false );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "AttributeDesc", "StrictDataMode" );

   assert.tryThrow( SDB_OPTION_NOT_SUPPORT, function()
   {
      cl.alter( {
         Alter: [{ Name: "enable sharding", Args: { ShardingKey: { b: 1 }, ShardingType: 'range' } },
         { Name: "set attributes", Args: { Name: "cs.cl" } },
         { Name: "set attributes", Args: { StrictDataMode: false } }],
         Options: { IgnoreException: false }
      } );
   } );

   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingKey", { b: 1 } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingType", "range" );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "AttributeDesc", "StrictDataMode" );

   cl.alter( { Alter: { Name: "enable sharding", Args: { ShardingKey: { a: -1 } } }, Options: { IgnoreException: true } } )
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingKey", { a: -1 } );

   commDropCL( db, csName, clName, true, false, "clean cl" );
}
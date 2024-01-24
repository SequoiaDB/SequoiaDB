/* *****************************************************************************
@discretion: parition cl alter ensureShardingIndex, the test scenario is as follows:
test a: alter ensureSharding from true to false
test b: alter ensureSharding from false to true
@author��2018-4-25 wuyan  Init
***************************************************************************** */
var clName1 = CHANGEDPREFIX + "_altercl_14977a";
var clName2 = CHANGEDPREFIX + "_altercl_14977b";

main( test );
function test ()
{
   if( true == commIsStandalone( db ) )
   {
      return;
   }
   //clean environment before test
   commDropCL( db, COMMCSNAME, clName1, true, true, "drop CL in the beginning" );
   commDropCL( db, COMMCSNAME, clName2, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl1 = commCreateCL( db, COMMCSNAME, clName1, { ShardingKey: { a: 1 }, ShardingType: "range" } );
   var dbcl2 = commCreateCL( db, COMMCSNAME, clName2, { ShardingKey: { a: 1 }, ShardingType: "range" } );;

   //test a: alter ensureShardingIndexfrom true to false, no alter shardingKey
   var ensureShardingIndex = false;
   dbcl1.setAttributes( { EnsureShardingIndex: ensureShardingIndex } );
   checkAlterResult( clName1, "EnsureShardingIndex", ensureShardingIndex );

   //test a: alter ensureShardingIndexfrom true to false, and alter shardingKey
   var ensureShardingIndex = false;
   dbcl2.setAttributes( { EnsureShardingIndex: ensureShardingIndex, ShardingKey: { b: 1 } } );
   checkAlterResult( clName2, "EnsureShardingIndex", ensureShardingIndex );
   checkShardIndex( dbcl2 );

   //test b: alter ensureShardingIndex from false to true
   var ensureShardingIndex = true;
   dbcl1.setAttributes( { EnsureShardingIndex: ensureShardingIndex } );
   checkAlterResult( clName1, "EnsureShardingIndex", ensureShardingIndex );

   //clean
   commDropCL( db, COMMCSNAME, clName1, true, true, "clear collection in the beginning" );
   commDropCL( db, COMMCSNAME, clName2, true, true, "clear collection in the beginning" );
}


function checkShardIndex ( dbcl )
{
   try
   {
      dbcl.getIndex( "$shard" );
      throw new Error( "need throw error" );
   }
   catch( e )
   {
      //-47:Index name does not exist
      if( e.message != SDB_IXM_NOTEXIST )
      {
         throw e;
      }
   }
}
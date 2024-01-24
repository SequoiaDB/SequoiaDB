/* *****************************************************************************
@discretion: cl alter ensureShardingIndex, the test scenario is as follows:
test a: add ensureShardingIndex
test b: add ensureShardingIndex and ShardingKey
@author��2018-4-25 wuyan  Init
***************************************************************************** */
var clName = CHANGEDPREFIX + "_altercl_14979";

main( test );
function test ()
{
   if( true == commIsStandalone( db ) )
   {
      return;
   }
   //clean environment before test
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   //test a: alter ensureShardingIndex, and no shardingKey
   alterEnsureShardingNoShardingKey( dbcl );

   //test b: alter ensureShardingIndex, and add shardingKey
   var ensureShardingIndex = true;
   dbcl.setAttributes( { EnsureShardingIndex: ensureShardingIndex, ShardingKey: { a: 1 } } );
   checkAlterResult( clName, "EnsureShardingIndex", ensureShardingIndex );
   checkShardIndex( dbcl );

   //clean
   commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the beginning" );
}


function alterEnsureShardingNoShardingKey ( dbcl )
{
   try
   {
      dbcl.setAttributes( ( { EnsureShardingIndex: true } ) );
      throw new Error( "need throw error" );
   }
   catch( e )
   {
      if( e.message != SDB_NO_SHARDINGKEY )
      {
         throw e;
      }
   }
}

function checkShardIndex ( cl )
{
   var timeout = 10;
   var time = 0;
   getIndex = cl.getIndex( "$shard" );
   while( undefined == getIndex && time < timeout )
   {
      getIndex = cl.getIndex( indexName );
      ++time;
      sleep( 1000 );
   }

   var indexDef = getIndex.toObj()["IndexDef"];
   var expKeyValue = { a: 1 };
   var name = "$shard";
   assert.equal( name, indexDef.name );
   assert.equal( expKeyValue, indexDef.key );
}
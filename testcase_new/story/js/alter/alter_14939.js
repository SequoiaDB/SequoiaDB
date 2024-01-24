/* *****************************************************************************
@discretion: alter shardingKey field equals to the index field
@author��2018-4-25 wuyan  Init
***************************************************************************** */
var clName = CHANGEDPREFIX + "_alterclShardingKey_14939";

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
   var dbcl = commCreateCL( db, COMMCSNAME, clName, { ShardingKey: { a: 1, b: 1 } } );;

   //create index, then alter shardingKey the same as index field
   var shardingKeyField = { c: 1 };
   dbcl.createIndex( "testindex", shardingKeyField );
   dbcl.enableSharding( { ShardingKey: shardingKeyField } );
   checkAlterResult( clName, "ShardingKey", shardingKeyField );
   checkShardIndex( dbcl );

   //clean
   commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the beginning" );
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
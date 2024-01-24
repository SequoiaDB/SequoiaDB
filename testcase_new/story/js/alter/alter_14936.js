/* *****************************************************************************
@discretion: cl exists unique index, alter shardingKey
@author��2018-4-25 wuyan  Init
***************************************************************************** */
var clName1 = CHANGEDPREFIX + "_alterclShardingKey_14936a";
var clName2 = CHANGEDPREFIX + "_alterclShardingKey_14936b";

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
   var dbcl1 = commCreateCL( db, COMMCSNAME, clName1 );
   var dbcl2 = commCreateCL( db, COMMCSNAME, clName2, { ShardingKey: { a: 1 } } );;

   //test a :add cl1 shardingKey, unique index no include all shardingKey all fields
   noParitionCLAlter( dbcl1 )

   //test b: alter cl2 shardingKey, unique index include all shardingKey fields
   paritionCLAlter( dbcl2, clName2 )

   //clean
   commDropCL( db, COMMCSNAME, clName1, true, true, "clear collection in the beginning" );
   commDropCL( db, COMMCSNAME, clName2, true, true, "clear collection in the beginning" );
}


function noParitionCLAlter ( dbcl )
{
   dbcl.createIndex( "testindex", { a: 1, c: 1 }, true );
   try
   {
      var shardingKeyField = { a: 1, b: 1 };
      dbcl.setAttributes( { ShardingKey: shardingKeyField } );
      throw new Error( "need throw error" );
   }
   catch( e )
   {
      if( e.message != SDB_SHARD_KEY_NOT_IN_UNIQUE_KEY )
      {
         throw e;
      }
   }
}

function paritionCLAlter ( dbcl, clName )
{

   dbcl.createIndex( "testindex", { a: 1, b: -1, c: 1 }, true );
   var shardingKeyField = { c: 1, b: 1 };
   dbcl.setAttributes( { ShardingKey: shardingKeyField } );
   checkAlterResult( clName, "ShardingKey", shardingKeyField );

}
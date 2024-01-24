/* *****************************************************************************
@discretion: parition cl alter shardingKey, the test scenario is as follows:
test a: alter sharding key of one field by enableSharding
test b: alter sharding key of multi field by enableSharding
test c: alter sharding key by setAttributes
@author��2018-4-25 wuyan  Init
***************************************************************************** */
var clName = CHANGEDPREFIX + "_alterclShardingKey_14937";

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

   //test a: alter sharding key of one field by enableSharding
   var shardingKeyField = { c: 1 };
   dbcl.enableSharding( { ShardingKey: shardingKeyField } );
   checkResult( clName, shardingKeyField );

   //test b: alter sharding key of multi field by enableSharding
   var shardingKeyField1 = { b: 1, c: -1, 'test': 1, 'test2': -1 };
   dbcl.enableSharding( { ShardingKey: shardingKeyField1 } );
   checkResult( clName, shardingKeyField1 );

   //test c: alter sharding key by setAttributes
   var shardingKeyField2 = { b: 1, c: -1, a: 1 };
   dbcl.enableSharding( { ShardingKey: shardingKeyField2 } );
   checkResult( clName, shardingKeyField2 );

   //clean
   commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the beginning" );
}


function checkResult ( clName, expShardingKeyField )
{
   var clFullName = COMMCSNAME + "." + clName;
   var cur = db.snapshot( 8, { "Name": clFullName } );
   var actualShardingKeyField = cur.current().toObj()["ShardingKey"];

   assert.equal( expShardingKeyField, actualShardingKeyField );
}
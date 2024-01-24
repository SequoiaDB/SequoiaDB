/* *****************************************************************************
@discretion: parition cl alter shardingType, the test scenario is as follows:
test a: alter shardingType from range to hash
test b: alter shardingType from hash to range
@author��2018-4-25 wuyan  Init
***************************************************************************** */
var clName = CHANGEDPREFIX + "_alterclShardingType_14949";

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
   var dbcl = commCreateCL( db, COMMCSNAME, clName, { ShardingKey: { a: 1, b: 1 }, ShardingType: "range" } );;

   //test a: alter shardingType from range to hash
   var shardingType = "range";
   dbcl.enableSharding( { ShardingKey: { a: 1, b: 1 }, ShardingType: shardingType } );
   checkAlterResult( clName, "ShardingType", shardingType );

   //test b: alter shardingType from hash to range
   var shardingType1 = "hash";
   dbcl.enableSharding( { ShardingKey: { a: 1, b: 1 }, ShardingType: shardingType1 } );
   checkAlterResult( clName, "ShardingType", shardingType1 );

   //clean
   commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the beginning" );
}

function checkResult ( clName, fieldName, expFieldValue )
{
   var clFullName = COMMCSNAME + "." + clName;
   var cur = db.snapshot( 8, { "Name": clFullName } );
   var actualFieldValue = cur.current().toObj()[fieldName];
   assert.equal( expFieldValue, actualFieldValue );
}
/* *****************************************************************************
@discretion: testcase15034/15036
cl alter index and replSize/strictDataMode, the test scenario is as follows:
test a: alter AutoIndex
test b: alter ensureShardingIndex
test c: alter replsize
test d: alter strictDataMode
alter() only test alter field value; 
@author��2018-4-27 wuyan  Init
***************************************************************************** */
var clName = CHANGEDPREFIX + "_altercl_15034";

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
   var dbcl = commCreateCL( db, COMMCSNAME, clName, { ShardingKey: { a: 1 }, AutoIndexId: false, EnsureShardingIndex: false } );

   //alter cl
   var autoIndexId = true
   var ensureShardingIndex = true;
   var replSize = 2;
   var strictDataMode = true;
   dbcl.alter( { EnsureShardingIndex: ensureShardingIndex, AutoIndexId: autoIndexId, ReplSize: replSize, StrictDataMode: strictDataMode } );

   //check alter result
   checkAlterResult( clName, "EnsureShardingIndex", ensureShardingIndex );
   checkIdIndexResult( dbcl );
   checkAlterResult( clName, "ReplSize", replSize );
   checkAlterResult( clName, "AttributeDesc", "Compressed | StrictDataMode" );

   //clean
   commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the beginning" );
}

function checkIdIndexResult ( cl )
{
   var timeout = 10;
   var time = 0;
   var name = "$id";
   getIndex = cl.getIndex( name );
   while( undefined == getIndex && time < timeout )
   {
      getIndex = cl.getIndex( name );
      ++time;
      sleep( 1000 );
   }

   var indexDef = getIndex.toObj()["IndexDef"];
   var expKeyValue = { _id: 1 };

   assert.equal( name, indexDef.name );
   assert.equal( expKeyValue, indexDef.key );
}
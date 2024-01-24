/* *****************************************************************************
@discretion: cl add shardingKey
@author��2018-4-16 wuyan  Init
***************************************************************************** */
var clName1 = CHANGEDPREFIX + "_alterclShardingKey_14935a";
var clName2 = CHANGEDPREFIX + "_alterclShardingKey_14935b";

main( test );
function test ()
{
   if( true == commIsStandalone( db ) )
   {
      return;
   }
   var allGroupName = getGroupName( db, true );
   if( 1 === allGroupName.length )
   {
      return;
   }
   var sourceGroup = allGroupName[0];
   var targetGroup = allGroupName[1];

   //clean environment before test
   commDropCL( db, COMMCSNAME, clName1, true, true, "drop CL in the beginning" );
   commDropCL( db, COMMCSNAME, clName2, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl1 = commCreateCL( db, COMMCSNAME, clName1, { Group: sourceGroup } );
   var dbcl2 = commCreateCL( db, COMMCSNAME, clName2, { Group: sourceGroup } );

   //preset data
   var data = [];
   for( var i = 0; i < 2000; i++ )
   {
      data.push( { a: i, b: i, c: "test record: " + i } );
   }
   dbcl1.insert( data );
   dbcl2.insert( data );

   //test a :alter cl1, one shardingKey field
   var shardingKeyField1 = { a: 1 };
   dbcl1.setAttributes( { ShardingKey: shardingKeyField1 } );
   checkAlterResult( clName1, "ShardingKey", shardingKeyField1 );
   dbcl1.split( sourceGroup, targetGroup, 50 );

   //test b: alter cl2, many shardingKey field
   var shardingKeyField2 = { a: 1, b: 1, c: -1 };
   dbcl2.setAttributes( { ShardingKey: shardingKeyField2 } );
   checkAlterResult( clName2, "ShardingKey", shardingKeyField2 );
   dbcl2.split( sourceGroup, targetGroup, 50 );

   //clean
   commDropCL( db, COMMCSNAME, clName1 );
   commDropCL( db, COMMCSNAME, clName2 );
}
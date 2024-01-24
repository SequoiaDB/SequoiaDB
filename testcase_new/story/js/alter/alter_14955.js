/* *****************************************************************************
@discretion: cl alter partition
@author��2018-4-25 wuyan  Init
***************************************************************************** */
var clName = CHANGEDPREFIX + "_alterclpartition_14955";

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

   //test a :alter parition, no shardingKey
   alterParitionNoShardingKey( dbcl );

   //test b: alter parition and shardingKey
   var shardingKey = { a: 1, b: 1 };
   var parition = 2048;
   dbcl.setAttributes( { ShardingKey: shardingKey, Partition: parition } );
   checkAlterResult( clName, "ShardingKey", shardingKey );
   checkAlterResult( clName, "Partition", parition );

   //clean
   commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the beginning" );
}


function alterParitionNoShardingKey ( dbcl )
{
   try
   {
      dbcl.setAttributes( { Paritition: 512 } );
      throw new Error( "need throw error" );
   }
   catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }
}
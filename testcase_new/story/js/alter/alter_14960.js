/* *****************************************************************************
@discretion: cl alter AutoSplit
@author��2018-4-25 wuyan  Init
***************************************************************************** */
var clName = CHANGEDPREFIX + "_alterclautosplit_14960";

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

   //test a :alter autosplit, no shardingKey
   alterAutoSplitNoShardingKey( dbcl );

   //test b: alter autosplit and shardingKey
   var shardingKey = { a: 1, b: 1 };
   var autoSplit = true;
   dbcl.setAttributes( { ShardingKey: shardingKey, AutoSplit: autoSplit } );
   checkAlterResult( clName, "ShardingKey", shardingKey );
   checkAlterResult( clName, "AutoSplit", autoSplit );

   //clean
   commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the beginning" );
}

function alterAutoSplitNoShardingKey ( dbcl )
{
   try
   {
      dbcl.setAttributes( { AutoSplit: true } );
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
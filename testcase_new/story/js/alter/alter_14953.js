/* *****************************************************************************
@discretion: maincl alter shardingKey and shardingType, the test scenario is as follows:
test a: alter shardingType
test b: alter shardingKey, no attach subcl
test c: alter shardingKey, attach subcl
@author��2018-4-25 wuyan  Init
***************************************************************************** */
var mainCLName = CHANGEDPREFIX + "_qalterMaincl_14953";
var subCLName = CHANGEDPREFIX + "_qalterSubcl_14953";

main( test );
function test ()
{
   if( true == commIsStandalone( db ) )
   {
      return;
   }
   //clean environment before test
   commDropCL( db, COMMCSNAME, mainCLName, true, true, "drop CL in the beginning" );

   //create subcl
   var mainCL = commCreateCL( db, COMMCSNAME, mainCLName, { ShardingKey: { a: 1 }, IsMainCL: true } );
   var subcl = commCreateCL( db, COMMCSNAME, subCLName, { ShardingKey: { b: 1 }, ShardingType: "range" } );

   //test a: alter shardingType
   alterShardingType( mainCL );

   //test b: alter shardingKey, no attach subcl
   var shardingKey = { time: 1 };
   mainCL.setAttributes( { ShardingKey: shardingKey } );
   checkAlterResult( mainCLName, "ShardingKey", shardingKey );

   //test c: alter shardingKey, attach subcl, can not alter shardingKey
   alterShardingKey( mainCL );

   //clean
   commDropCL( db, COMMCSNAME, subCLName, true, true, "clear collection in the beginning" );
   commDropCL( db, COMMCSNAME, mainCLName, true, true, "clear collection in the beginning" );
}


function alterShardingType ( dbcl )
{
   try
   {
      dbcl.setAttributes( { ShardingType: "hash" } );
      throw new Error( "need throw error" );
   }
   catch( e )
   {
      if( e.message != SDB_OPTION_NOT_SUPPORT )
      {
         throw e;
      }
   }
}

function alterShardingKey ( dbcl )
{
   try
   {
      var options = { LowBound: { "time": { $minKey: 1 } }, UpBound: { "time": { $maxKey: 1 } } };
      dbcl.attachCL( COMMCSNAME + "." + subCLName, options );
      dbcl.setAttributes( { ShardingKey: { a: 1 } } );
      throw new Error( "need throw error" );
   }
   catch( e )
   {
      if( e.message != SDB_OPTION_NOT_SUPPORT )
      {
         throw e;
      }
   }
}

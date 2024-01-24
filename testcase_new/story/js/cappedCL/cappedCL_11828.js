/************************************
*@Description: 创建固定集合空间集合，find查询数据 
*@author:      luweikang
*@createdate:  2017.7.12
*@testlinkCase:seqDB-11828
**************************************/

main( test );
function test ()
{
   if( true === commIsStandalone( db ) )
   {
      return;
   }

   var clName1 = COMMCAPPEDCLNAME + "_11828_CL1";
   var clName2 = COMMCAPPEDCLNAME + "_11828_CL2";
   var clName3 = COMMCAPPEDCLNAME + "_11828_CL3";

   //check cappedCL sharding
   var hashOptions = { Capped: true, Size: 1024, Max: 10000000, AutoIndexId: false, ShardingKey: { "age": 1 }, ShardingType: "hash", Partition: 1024 };
   checkCreateCLOptions( COMMCAPPEDCSNAME, clName1, hashOptions, true );

   var rangeOptions = { Capped: true, Size: 1024, Max: 10000000, AutoIndexId: false, ShardingKey: { "age": 1 }, ShardingType: "hash", Partition: 1024 };
   checkCreateCLOptions( COMMCAPPEDCSNAME, clName2, rangeOptions, true );

   //check cappedCL alter
   var options = { Capped: true, Size: 1024, Max: 10000000, AutoIndexId: false };
   var dbcl = commCreateCL( db, COMMCAPPEDCSNAME, clName3, options, false, false, "create capped cl" );
   var alterOption1 = { ShardingKey: { a: 1 }, ShardingType: "hash" };
   checkCappedAlter( dbcl, alterOption1 );
   var alterOption2 = { ShardingKey: { a: 1 }, ShardingType: "range" };
   checkCappedAlter( dbcl, alterOption2 );
   checkSnapshot( COMMCAPPEDCSNAME, clName3 );

   //clean environment after test  
   commDropCL( db, COMMCAPPEDCSNAME, clName1, true, true, "drop CL in the end" );
   commDropCL( db, COMMCAPPEDCSNAME, clName2, true, true, "drop CL in the end" );
   commDropCL( db, COMMCAPPEDCSNAME, clName3, true, true, "drop CL in the end" );
}

function checkCreateCLOptions ( csName, clName, options, isValid )
{
   try
   {
      db.getCS( csName ).createCL( clName, options );
      assert.notEqual( isValid, undefined );
   }
   catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }
}

function checkCappedAlter ( dbcl, options )
{
   assert.tryThrow( SDB_OPTION_NOT_SUPPORT, function()
   {
      dbcl.alter( options );
   } );

}

function checkSnapshot ( csName, clName )
{
   var cl_full_name = csName + '.' + clName;
   var options = { Name: cl_full_name };
   var rec = db.snapshot( 8, options );
   var shardingType = rec.current().toObj().ShardingType;
   var shardingKey = rec.current().toObj().ShardingKey;
   if( shardingType != undefined || shardingKey != undefined )
   {
      throw new Error( "CHECK SNAPSHOT FAILED" );
   }
}
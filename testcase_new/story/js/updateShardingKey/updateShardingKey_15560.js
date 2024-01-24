/************************************
*@Description: shardedCL update ShardingKey,
                  the ShardingKey are updated fail
*@author:      wuyan
*@createdate:  2018.7.29
**************************************/
var clName = CHANGEDPREFIX + "_updateShardingKey_15560";
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
   var shardingKey = { no: 1 };
   var dbcl = createCL( COMMCSNAME, clName, shardingKey );

   //insert data 	
   var doc = [{ no: "testupdate", a: "testa3", b: 3 }];
   dbcl.insert( doc );

   //update ShardingKey,set KeepShardingKey=true
   var updateCondition = { $set: { no: "testupdate", a: "testa" } };
   assert.tryThrow( SDB_UPDATE_SHARD_KEY, function()
   {
      dbcl.update( updateCondition, {}, {}, { KeepShardingKey: true } );
   } );

   var findCondition = { b: { $gte: 2 } };
   var upsertCondition = { $set: { no: "testupdate12807", a: "testa12807" } };
   assert.tryThrow( SDB_UPDATE_SHARD_KEY, function()
   {
      dbcl.upsert( upsertCondition, findCondition, {}, {}, { KeepShardingKey: true } );
   } );

   //check the update result	
   checkResult( dbcl, null, null, doc, { _id: 1 } );

   // drop collectionspace in clean
   commDropCL( db, COMMCSNAME, clName, false, false,
      "drop colleciton in the end" );

}

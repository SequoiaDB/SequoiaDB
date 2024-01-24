/************************************
*@Description: shardedCL upsert ShardingKey,the records exclusive shardingKey
*@author:      wuyan
*@createdate:  2017.10.08
**************************************/
var clName = CHANGEDPREFIX + "_updateShardingKey_12807";
//main( test );
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
   var doc = [{ a: "testa1", b: 1 },
   { a: ["test1", "test2"], b: 2 }];
   dbcl.insert( doc );

   //findAndUpdate ShardingKey,set KeepShardingKey=true
   var findCondition = { b: { $lte: 2 } };
   var upsertCondition = { $set: { no: "testupdate12807", a: "testa12807" } };
   upsertData( dbcl, upsertCondition, findCondition, {}, {}, true );

   //check the update result
   var expRecs = [{ no: "testupdate12807", a: "testa12807", b: 1 },
   { no: "testupdate12807", a: "testa12807", b: 2 }];
   checkResult( dbcl, null, null, expRecs, { _id: 1 } );

   // drop collectionspace in clean
   commDropCL( db, COMMCSNAME, clName, false, false,
      "drop colleciton in the end" );

}

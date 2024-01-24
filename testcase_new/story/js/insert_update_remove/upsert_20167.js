/******************************************************************************
*@Description : seqDB-20167: upsert字段全为分区键   
*@Author      : 2020-01-09  Zhao xiaoni Init
******************************************************************************/
testConf.skipStandAlone = true;
testConf.csName = COMMCSNAME;
testConf.clName = COMMCLNAME + "_20167";
testConf.clOpt = { "ShardingType": "hash", "ShardingKey": { "_id": 1, "a": 1 } };

main( test );

function test ()
{
   var cl = db.getCS( testConf.csName ).getCL( testConf.clName );

   var idStartData = 0;
   var aStartData = 0;
   var dataNum = 10;
   var doc = getBulkData( dataNum, idStartData, aStartData );
   cl.insert( doc );

   //更新一条记录
   var updatedRecord = { "_id": 10, "a": 10 };
   var actRecs = cl.upsert( { "$set": updatedRecord }, doc[9] ).toObj();
   var expRecs = { "UpdatedNum": 1, "ModifiedNum": 0, "InsertedNum": 0 };
   if( !commCompareObject( expRecs, actRecs ) )
   {
      throw new Error( "\nactRecs: " + JSON.stringify( actRecs ) + "\nexpRecs: " + JSON.stringify( expRecs ) );
   }

   //更新多条记录
   var actRecs = cl.upsert( { "$set": updatedRecord } ).toObj();
   var expRecs = { "UpdatedNum": 10, "ModifiedNum": 0, "InsertedNum": 0 };
   if( !commCompareObject( expRecs, actRecs ) )
   {
      throw new Error( "\nactRecs: " + JSON.stringify( actRecs ) + "\nexpRecs: " + JSON.stringify( expRecs ) );
   }

   //更新一条记录，KeepShardingKey为true
   assert.tryThrow( SDB_UPDATE_SHARD_KEY, function()
   {
      cl.upsert( { "$set": updatedRecord }, doc[9], {}, {}, { "KeepShardingKey": true } );
   } );

   //更新多条记录，KeepShardingKey为true
   assert.tryThrow( SDB_UPDATE_SHARD_KEY, function()
   {
      cl.upsert( { "$set": updatedRecord }, {}, {}, {}, { "KeepShardingKey": true } );
   } );
}

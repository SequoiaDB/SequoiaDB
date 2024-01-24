/******************************************************************************
*@Description : seqDB-20162: 更新字段部分为分区键部分为普通字段
*@Author      : 2020-01-09  Zhao xiaoni Init
******************************************************************************/
testConf.skipStandAlone = true;
testConf.csName = COMMCSNAME;
testConf.clName = COMMCLNAME + "_20162";
testConf.clOpt = { "ShardingType": "hash", "ShardingKey": { "_id": 1 } };

main( test );

function test ()
{
   var cl = db.getCS( testConf.csName ).getCL( testConf.clName );

   var idStartData = 0;
   var aStartData = 0;
   var dataNum = 10;
   var doc = getBulkData( dataNum, idStartData, aStartData );
   cl.insert( doc );

   //更新一条记录，部分分区键，部分非分区键
   var updatedRecord = { "_id": 10, "a": 10 };
   var actRecs = cl.update( { "$set": updatedRecord }, doc[9] ).toObj();
   var expRecs = { "UpdatedNum": 1, "ModifiedNum": 1, "InsertedNum": 0 };
   if( !commCompareObject( expRecs, actRecs ) )
   {
      throw new Error( "\nactRecs: " + JSON.stringify( actRecs ) + "\nexpRecs: " + JSON.stringify( expRecs ) );
   }

   //更新多条记录，部分分区键，部分非分区键
   updatedRecord = { "_id": 11, "a": 11 };
   expRecs = { "UpdatedNum": 10, "ModifiedNum": 10, "InsertedNum": 0 };
   actRecs = cl.update( { "$set": updatedRecord } ).toObj();
   if( !commCompareObject( expRecs, actRecs ) )
   {
      throw new Error( "\nactRecs: " + JSON.stringify( actRecs ) + "\nexpRecs: " + JSON.stringify( expRecs ) );
   }

   //更新多条记录，部分分区键，部分非分区键，KeepShardingKey为true
   assert.tryThrow( SDB_UPDATE_SHARD_KEY, function()
   {
      cl.update( { "$set": updatedRecord }, {}, {}, { "KeepShardingKey": true } );
   } );
}

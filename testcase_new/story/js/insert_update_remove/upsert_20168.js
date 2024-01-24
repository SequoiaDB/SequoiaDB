/******************************************************************************
*@Description : seqDB-20168: upsert字段部分为分区键部分为普通字段   
*@Author      : 2020-01-09  Zhao xiaoni Init
******************************************************************************/
testConf.skipStandAlone = true;
testConf.csName = COMMCSNAME;
testConf.clName = COMMCLNAME + "_20168";
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

   //更新一条记录，更新条件匹配的记录能在集合中找到
   var updatedRecord = { "_id": 10, "a": 10 };
   var actRecs = cl.upsert( { "$set": updatedRecord }, doc[9] ).toObj();
   var expRecs = { "UpdatedNum": 1, "ModifiedNum": 1, "InsertedNum": 0 };
   if( !commCompareObject( expRecs, actRecs ) )
   {
      throw new Error( "\nactRecs: " + JSON.stringify( actRecs ) + "\nexpRecs: " + JSON.stringify( expRecs ) );
   }

   //更新多条记录，更新条件匹配的记录能在集合中找到
   var updatedRecord = { "_id": 11, "a": 11 };
   var actRecs = cl.upsert( { "$set": updatedRecord } ).toObj();
   var expRecs = { "UpdatedNum": 10, "ModifiedNum": 10, "InsertedNum": 0 };
   if( !commCompareObject( expRecs, actRecs ) )
   {
      throw new Error( "\nactRecs: " + JSON.stringify( actRecs ) + "\nexpRecs: " + JSON.stringify( expRecs ) );
   }

   //更新一条记录，更新条件匹配的记录不能在集合中找到
   var updatedRecord = { "_id": 12, "a": 12 };
   var actRecs = cl.upsert( { "$set": updatedRecord }, updatedRecord ).toObj();
   var expRecs = { "UpdatedNum": 0, "ModifiedNum": 0, "InsertedNum": 1 };
   if( !commCompareObject( expRecs, actRecs ) )
   {
      throw new Error( "\nactRecs: " + JSON.stringify( actRecs ) + "\nexpRecs: " + JSON.stringify( expRecs ) );
   }

   //更新多条记录，更新条件匹配的部分记录不能在集合中找到
   var updatedRecord = { "c": 13 };
   var actRecs = cl.upsert( { "$set": updatedRecord }, { "_id": { "$gt": 3 } } ).toObj();
   var expRecs = { "UpdatedNum": 7, "ModifiedNum": 7, "InsertedNum": 0 };
   if( !commCompareObject( expRecs, actRecs ) )
   {
      throw new Error( "\nactRecs: " + JSON.stringify( actRecs ) + "\nexpRecs: " + JSON.stringify( expRecs ) );
   }
}

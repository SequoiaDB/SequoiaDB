/******************************************************************************
*@Description : seqDB-20164: upsert字段全为普通字段，且更新条件匹配的所有记录都存在集合中
                seqDB-20166: upsert字段全为普通字段，且更新条件匹配的所有记录都不存在集合中
                seqDB-20169: upsert插入重复数据
*@Author      : 2020-01-09  Zhao xiaoni Init
******************************************************************************/
testConf.csName = COMMCSNAME;
testConf.clName = COMMCLNAME + "_20164_20166_20169";

main( test );

function test ()
{
   var cl = db.getCS( testConf.csName ).getCL( testConf.clName );

   var idStartData = 0;
   var aStartData = 0;
   var dataNum = 10;
   var doc = getBulkData( dataNum, idStartData, aStartData );
   cl.insert( doc );

   //更新一条记录，且更新条件匹配的记录存在集合中
   var updatedRecord = { "_id": 10, "a": 10 };
   var actRecs = cl.upsert( { "$set": updatedRecord }, doc[9] ).toObj();
   var expRecs = { "UpdatedNum": 1, "ModifiedNum": 1, "InsertedNum": 0 };
   if( !commCompareObject( expRecs, actRecs ) )
   {
      throw new Error( "\nactRecs: " + JSON.stringify( actRecs ) + "\nexpRecs: " + JSON.stringify( expRecs ) );
   }

   //更新多条记录，且更新条件匹配的记录都存在集合中
   var updatedRecord = { "a": 11 };
   var actRecs = cl.upsert( { "$set": updatedRecord } ).toObj();
   var expRecs = { "UpdatedNum": 10, "ModifiedNum": 10, "InsertedNum": 0 };
   if( !commCompareObject( expRecs, actRecs ) )
   {
      throw new Error( "\nactRecs: " + JSON.stringify( actRecs ) + "\nexpRecs: " + JSON.stringify( expRecs ) );
   }

   //更新一条记录，且更新条件匹配的记录不存在集合中
   var updatedRecord = { "_id": 9, "a": 9 };
   var actRecs = cl.upsert( { "$set": updatedRecord }, updatedRecord ).toObj();
   var expRecs = { "UpdatedNum": 0, "ModifiedNum": 0, "InsertedNum": 1 };
   if( !commCompareObject( expRecs, actRecs ) )
   {
      throw new Error( "\nactRecs: " + JSON.stringify( actRecs ) + "\nexpRecs: " + JSON.stringify( expRecs ) );
   }

   //upsert插入唯一索引重复的数据
   assert.tryThrow( SDB_IXM_DUP_KEY, function()
   {
      var updatedRecord = { "_id": 9 }
      cl.upsert( { "$set": updatedRecord }, { "a": 500 } );
   } );
}

/******************************************************************************
*@Description : seqDB-15740:指定update查询记录
*@author      : chensiqin 2018-09-11  huangxiaoni 2020-10-12
******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_15740";
testConf.clOpt = { "ShardingKey": { "typeint": 1 }, "ShardingType": "hash" };

main( test );
function test ( arg )
{
   var cl = arg.testCL;
   cl.insert( { "typeint": 123, "typefloat": 123.456 } );

   // a、查询更新分区键字段，指定更新规则rule，设置返回更新后的记录，设置KeepShardingKey为true
   assert.tryThrow( SDB_UPDATE_SHARD_KEY, function()
   {
      cl.find( new SdbQueryOption().update( { "$inc": { "typeint": 1 }, "$set": { "typefloat": 1.3 } }, true, { "KeepShardingKey": true } ) );
   } );

   // b、查询更新非分区键字段，指定更新规则rule，设置返回更新后的记录，设置KeepShardingKey为false
   var cursor = cl.find( new SdbQueryOption().update( { "$set": { "typefloat": 1.4 } }, true, { "KeepShardingKey": false } ) );
   var size = 0;
   while( cursor.next() )
   {
      assert.equal( cursor.current().toObj().typefloat, 1.4 );
      size++;
   }
   assert.equal( size, 1 );

   // c、查询更新非分区键字段，指定更新规则rule，设置不返回更新后的记录，设置KeepShardingKey为false
   var cursor = cl.find( new SdbQueryOption().update( { "$inc": { "typefloat": 1 } }, false, { "KeepShardingKey": false } ) );
   var size = 0;
   while( cursor.next() )
   {
      assert.equal( cursor.current().toObj().typefloat, 1.4 );
      size++;
   }
   assert.equal( size, 1 );
}
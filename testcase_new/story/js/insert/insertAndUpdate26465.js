/******************************************************************************
 * @Description   : seqDB-26465:分区表指定更新规则，单条/批量插入冲突记录，涉及分区键修改
 * @Author        : Lin Yingting
 * @CreateTime    : 2022.05.13
 * @LastEditTime  : 2022.06.10
 * @LastEditors   : Lin Yingting
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_26465";
testConf.clOpt = { ShardingKey: { "a": 1 }, ShardingType: "range" };

main( test );

function test ( testPara )
{
   //分区表创建唯一索引，插入记录
   var cl = testPara.testCL;
   cl.createIndex( "idx_a", { a: 1 }, true, true );
   cl.insert( [{ a: 1, b: 1 }, { a: 2, b: 2 }] );

   //指定UpdateOnDup和更新规则，单条插入冲突记录，涉及分区键修改，分区键值不变
   cl.insert( { a: 1, b: 1 }, { UpdateOnDup: true, Update: { "$set": { "a": 1, "b": 2 } } } );
   var actRes = cl.find().sort( { a: 1 } );
   var expRes = [{ a: 1, b: 2 }, { a: 2, b: 2 }];
   commCompareResults( actRes, expRes );

   //指定UpdateOnDup和更新规则，批量插入冲突记录，涉及分区键修改，分区键值不变
   cl.insert( [{ a: 1, b: 2 }, { a: 2, b: 2 }], { UpdateOnDup: true, Update: { "$inc": { "a": 0, "b": 1 } } } );
   actRes = cl.find().sort( { a: 1 } );
   expRes = [{ a: 1, b: 3 }, { a: 2, b: 3 }];
   commCompareResults( actRes, expRes );

   //指定UpdateOnDup和更新规则，单条插入冲突记录，涉及分区键修改，分区键值变化
   assert.tryThrow( SDB_UPDATE_SHARD_KEY, function ()
   {
      cl.insert( { a: 1, b: 3 }, { UpdateOnDup: true, Update: { "$inc": { "a": 1 } } } );
   } );

   //指定UpdateOnDup和更新规则，批量插入冲突记录，涉及分区键修改，分区键值变化
   assert.tryThrow( SDB_UPDATE_SHARD_KEY, function ()
   {
      cl.insert( [{ a: 1, b: 3 }, { a: 2, b: 3 }], { UpdateOnDup: true, Update: { "$set": { "a": 1 } } } );
   } );

   //检查表数据
   actRes = cl.find().sort( { a: 1 } );
   expRes = [{ a: 1, b: 3 }, { a: 2, b: 3 }];
   commCompareResults( actRes, expRes );
}

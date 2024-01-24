/******************************************************************************
 * @Description   : seqDB-26467:分区表指定更新规则批量插入冲突记录，涉及多个数据组，
 *                              有/无事务，更新后记录冲突
 * @Author        : Lin Yingting
 * @CreateTime    : 2022.05.16
 * @LastEditTime  : 2022.06.10
 * @LastEditors   : Lin Yingting
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;
testConf.clName = COMMCLNAME + "_26467";
testConf.clOpt = { ShardingKey: { "b": 1 }, ShardingType: "range" };
testConf.useSrcGroup = true;
testConf.useDstGroup = true;

main( test );

function test ( testPara )
{
   var cl = testPara.testCL;
   var srcGroupName = testPara.srcGroupName;
   var dstGroupName = testPara.dstGroupNames[0];

   //创建分区表，创建唯一索引，插入记录，切分记录到多个数据组
   cl.createIndex( "idx_a", { a: 1, b: 1 }, true, true );
   cl.insert( [{ a: 1, b: 1 }, { a: 2, b: 1 }, { a: 10, b: 10 }, { a: 11, b: 10 }] );
   cl.split( srcGroupName, dstGroupName, { b: 10 }, { b: 20 } );

   //有事务，更新后记录部分冲突
   db.transBegin();
   assert.tryThrow( SDB_IXM_DUP_KEY, function ()
   {
      cl.insert( [{ a: 2, b: 1 }, { a: 10, b: 10 }], { UpdateOnDup: true, Update: { "$inc": { "a": 1 } } } );
   } );
   db.transCommit();
   var actRes = cl.find().sort( { a: 1 } );
   var expRes = [{ a: 1, b: 1 }, { a: 2, b: 1 }, { a: 10, b: 10 }, { a: 11, b: 10 }];
   commCompareResults( actRes, expRes );

   //无事务，更新后记录部分冲突
   assert.tryThrow( SDB_IXM_DUP_KEY, function ()
   {
      cl.insert( [{ a: 2, b: 1 }, { a: 10, b: 10 }], { UpdateOnDup: true, Update: { "$inc": { "a": 1 } } } );
   } );
   actRes = cl.find().sort( { a: 1 } );
   expRes = [{ a: 1, b: 1 }, { a: 3, b: 1 }, { a: 10, b: 10 }, { a: 11, b: 10 }];
   commCompareResults( actRes, expRes );
}

/******************************************************************************
 * @Description   : seqDB-26466:分区表指定更新规则，批量插入冲突记录，涉及多个数据组，更新后记录不冲突
 * @Author        : Lin Yingting
 * @CreateTime    : 2022.05.13
 * @LastEditTime  : 2022.05.13
 * @LastEditors   : Lin Yingting
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;
testConf.clName = COMMCLNAME + "_26466";
testConf.clOpt = { ShardingKey: { "b": 1 }, ShardingType: "range" };
testConf.useSrcGroup = true;
testConf.useDstGroup = true;

main( test );

function test ( testPara )
{
   var cl = testPara.testCL;
   var srcGroupName = testPara.srcGroupName;
   var dstGroupName = testPara.dstGroupNames[0];

   //创建唯一索引，插入记录，切分记录到多个数据组
   cl.createIndex( "idx_a", { a: 1, b: 1 }, true, true );
   cl.insert( [{ a: 1, b: 1 }, { a: 11, b: 11 }] );
   cl.split( srcGroupName, dstGroupName, { b: 10 }, { b: 20 } );

   //指定更新规则，批量插入冲突记录，涉及多个数据组，更新后记录不冲突
   cl.insert( [{ a: 1, b: 1 }, { a: 11, b: 11 }], { UpdateOnDup: true, Update: { "$inc": { "a": 1 } } } );
   var actRes = cl.find().sort( { a: 1 } );
   var expRes = [{ a: 2, b: 1 }, { a: 12, b: 11 }];
   commCompareResults( actRes, expRes );
}

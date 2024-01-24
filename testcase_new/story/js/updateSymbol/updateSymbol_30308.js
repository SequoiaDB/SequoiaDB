/******************************************************************************
 * @Description   : seqDB-30308:update使用rename更新字段为shardingKey字段
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.02.24
 * @LastEditTime  : 2023.02.24
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "30308";
testConf.skipStandAlone = true;
main( test );

function test ( args )
{
   var cl = args.testCL;
   cl.enableSharding( { ShardingKey: { a: 1 } } );

   var docs = [];
   var recsNum = 100;
   for( var i = 0; i < recsNum; i++ )
   {
      docs.push( { a: i, b: i } );
   }
   cl.insert( docs );

   // 更新shardingKey字段为其他字段
   cl.update( { $rename: { "a": "c" } } )

   // 更新其他字段为shardingKey同名字段
   cl.update( { $rename: { "b": "a" } } )

   // 检查字段是否被更新
   var actResult = cl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );
}
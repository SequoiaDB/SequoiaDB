/******************************************************************************
@Description : seqDB-13743:hash表的排序（切分键和非切分键索引排序）
@Modify list :
               2015-01-16 pusheng Ding  Init
               2020-08-17 Zixian Yan    Modify
               2020-10-12 Xiaoni Huang  Modify
******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneDuplicatePerGroup = true;
testConf.useSrcGroup = true;
testConf.useDstGroup = true
testConf.clName = COMMCLNAME + "_13742";
testConf.clOpt = { "ShardingKey": { "a": 1 }, "ShardingType": 'hash' };

main( test );
function test ( testPara )
{
   var srcGroupName = testPara.srcGroupName;
   var dstGroupName = ( testPara.dstGroupNames )[0];
   var cl = testPara.testCL;
   var rownums = 10000;

   // 写数据
   var records = [];
   for( var i = 0; i < rownums; i++ )
   {
      records.push( { a: ( rownums - 1 ) - i, b: i, c: "abcdefghijkl" + i } );
   }
   cl.insert( records );

   // 切分
   cl.split( srcGroupName, dstGroupName, 50 );
   // 使用sort对分区键字段进行排序
   var cursor = cl.find( {}, { "_id": { "$include": 0 } } ).sort( { a: 1 } );
   var expRecs = [];
   for( var i = 0; i < rownums; i++ )
   {
      expRecs.push( { a: i, b: ( rownums - 1 ) - i, c: "abcdefghijkl" + ( rownums - 1 - i ) } );
   }
   commCompareResults( cursor, expRecs );

   // 使用sort对非分区键索引字段进行排序
   var indexName = "idx";
   cl.createIndex( indexName, { b: 1 } );
   var cursor = cl.find( {}, { "_id": { "$include": 0 } } ).sort( { b: -1 } ).hint( { "": indexName } );
   commCompareResults( cursor, expRecs );
}

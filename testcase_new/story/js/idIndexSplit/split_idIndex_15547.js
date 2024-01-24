/******************************************************************************
@Description : seqDB-15547:创建id索引后，执行切分
@Modify list : 2018-08-08  XiaoNi Zhao  Init
               2019-11-22  XiaoNi Huang  Modify
******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;
testConf.useSrcGroup = true;
testConf.useDstGroup = true;
testConf.clName = CHANGEDPREFIX + "_split15547";
testConf.clOpt = { "ShardingKey": { "a": 1 }, "ShardingType": "hash", "AutoIndexId": false };

main( test );
function test ( arg )
{
   var srcGroupName = arg.srcGroupName;
   var dstGroupName = arg.dstGroupNames[0];
   var recsNum = 100;
   var cl = arg.testCL;

   // insert
   var docs = [];
   for( var i = 0; i < recsNum; i++ )
   {
      docs.push( { "a": i } );
   }
   cl.insert( docs );

   // create idIndex, then split
   cl.createIdIndex();
   cl.split( srcGroupName, dstGroupName, 50 );

   // check results
   var cursor = cl.find( {}, { "_id": { "$include": 0 } } ).sort( { "a": 1 } );
   commCompareResults( cursor, docs );
   checkSplitResults( COMMCSNAME, testConf.clName, [srcGroupName, dstGroupName], recsNum );
}
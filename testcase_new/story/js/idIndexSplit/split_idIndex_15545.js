/******************************************************************************
@Description : seqDB-15545:指定AutoIndexId:false，加入域并使用自动切分
@Modify list : 2018-08-08  XiaoNi Zhao  Init
               2019-11-22  XiaoNi Huang  Modify
******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;
testConf.csName = CHANGEDPREFIX + "_split15545";
testConf.csOpt = { "Domain": CHANGEDPREFIX + "_split15545" };
testConf.clName = "cl";
testConf.clOpt = { "ShardingKey": { "a": 1 }, "ShardingType": "hash", "AutoIndexId": false, "AutoSplit": true };

main( test );
function test ( arg )
{
   var cl = arg.testCL;
   var recsNum = 100;

   // insert
   var docs = [];
   for( var i = 0; i < recsNum; i++ )
   {
      docs.push( { "a": i } );
   }
   cl.insert( docs );

   // check results
   var cursor = cl.find( {}, { "_id": { "$include": 0 } } ).sort( { "a": 1 } );
   commCompareResults( cursor, docs );
   checkSplitResults( testConf.csName, testConf.clName, arg.dmGroupNames, recsNum );
}

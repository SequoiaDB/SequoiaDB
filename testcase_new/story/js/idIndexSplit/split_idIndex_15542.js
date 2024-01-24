/******************************************************************************
@Description : seqDB-15542:指定AutoIndexId:false创建切分表，执行切分
@Modify list : 2018-08-06  XiaoNi Zhao  Init
               2019-11-22  XiaoNi Huang  Modify
******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;
testConf.useSrcGroup = true;
testConf.useDstGroup = true;
testConf.clName = CHANGEDPREFIX + "_split15542";
testConf.clOpt = { "ShardingKey": { "a": 1 }, "ShardingType": "range", "AutoIndexId": false };

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

   // split
   try
   {
      cl.split( srcGroupName, dstGroupName, 50 );
      throw new Error( "expected failure, actual return success." );
   }
   catch( e )
   {
      if( SDB_RTN_AUTOINDEXID_IS_FALSE != e.message )
      {
         throw e;
      }
   }

   // check results
   var cursor = cl.find( {}, { "_id": { "$include": 0 } } ).sort( { "a": 1 } );
   commCompareResults( cursor, docs );
   checkSplitResults( COMMCSNAME, testConf.clName, [srcGroupName], recsNum );
}

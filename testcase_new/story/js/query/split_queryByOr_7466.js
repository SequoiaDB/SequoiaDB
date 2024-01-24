/******************************************************************************
*@Description :  seqDB-7466:执行split，使用or操作匹配条件查询数据*
*@Modify list :
*               2014-07-17 pusheng Ding  Init
*               2020-08-13 wuyan  modify
******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;

testConf.useSrcGroup = true;
testConf.useDstGroup = true;
testConf.clOpt = { ShardingKey: { b: 1 }, ShardingType: 'range' };
testConf.clName = COMMCLNAME + "_cl_7466";
main( test );

function test ( testPara )
{
   var docs = [];
   for( var i = 0; i < 10000; i++ )
   {
      docs.push( { a: i - 10000, b: i, c: "abcdefghijkl" + i } );
   }
   testPara.testCL.insert( docs );

   testPara.testCL.createIndex( "idx_7466", { a: 1 }, false, false );

   var srcGroupName = testPara.srcGroupName;
   var dstGroupName = testPara.dstGroupNames[0];
   testPara.testCL.split( srcGroupName, dstGroupName, 50 );

   var cursor = testPara.testCL.find( { $or: [{ a: { $lte: -9000 } }, { a: { $gte: -1000 } }, { b: 2000 }] } ).sort( { a: 1 } );
   checkResult( cursor );
}

function checkResult ( cursor )
{
   var actRecs = [];
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      actRecs.push( obj );
      if( !( obj['a'] <= -9000 || obj['a'] >= -1000 || obj['b'] == 2000 ) )
      {
         throw new Error( "\nreturn incorrect record: " + JSON.stringify( obj ) );
      }
   }
   cursor.close();

   var expCount = 2002;
   assert.equal( actRecs.length, expCount );
}

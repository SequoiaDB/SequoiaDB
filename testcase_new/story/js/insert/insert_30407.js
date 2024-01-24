/******************************************************************************
 * @Description   : seqDB-30407:分区表插入数据指定SDB_INSERT_CONTONDUP_ID或SDB_INSERT_REPLACEONDUP_ID
 * @Author        : liuli
 * @CreateTime    : 2023.03.10
 * @LastEditTime  : 2023.03.10
 * @LastEditors   : liuli
******************************************************************************/
testConf.clName = COMMCLNAME + "_30407";
testConf.clOpt = { ShardingKey: { "_id": 1 }, AutoSplit: true };
testConf.skipStandAlone = true;
main( test );

function test ( testPara )
{
   var cl = testPara.testCL;

   var docs = [];
   var expRecs = [];
   for( var i = 0; i < 1000; i++ )
   {
      docs.push( { "_id": i, a: i } );
      expRecs.push( { "_id": i, a: i } );
   }
   cl.insert( docs );

   // test flag:SDB_INSERT_CONTONDUP_ID
   docs = [];
   for( var i = 900; i < 1100; i++ )
   {
      docs.push( { "_id": i, b: i } );
      if( i >= 1000 )
      {
         expRecs.push( { "_id": i, b: i } );
      }
   }
   cl.insert( docs, SDB_INSERT_CONTONDUP_ID );
   var actRecs = cl.find().sort( { "_id": 1 } );
   commCompareResults( actRecs, expRecs, false );

   // test flag:SDB_INSERT_REPLACEONDUP_ID
   docs = [];
   expRecs = expRecs.slice( 0, 900 );
   for( var i = 900; i < 1200; i++ )
   {
      docs.push( { "_id": i, c: i } );
      expRecs.push( { "_id": i, c: i } );
   }
   cl.insert( docs, SDB_INSERT_REPLACEONDUP_ID );
   var actRecs = cl.find().sort( { "_id": 1 } );
   commCompareResults( actRecs, expRecs, false );
}

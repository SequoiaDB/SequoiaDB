/******************************************************************************
*@Description : seqDB-12432:查询时多个索引符合候选计划要求的索引选择
*@author      : Li Yuanyue
*@Date        : 2020.5.12
******************************************************************************/
testConf.clName = CHANGEDPREFIX + "_12432";

main( test );

function test ( args )
{
   var idxName1 = "index_a_12432";
   var idxName2 = "index_b_12432";
   var fullclName = COMMCSNAME + "." + testConf.clName;

   var cl = args.testCL;

   cl.createIndex( idxName2, { b: 1 } );
   cl.createIndex( idxName1, { a: 1 } );

   // 100W条
   for( var i = 0; i < 100; i++ )
   {
      var docs = [];
      for( var j = 1; j <= 10000; j++ )
      {
         docs.push( { a: ( i * 10000 + j ), b: ( i * 100000 + j ) } );
      }
      cl.insert( docs );
   }

   db.analyze( { Collection: fullclName } );

   var cond = { $and: [{ $and: [{ b: { $gt: 1 } }, { b: { $lt: 1000000 } }] }, { a: 1 }] };
   var expIndexName = idxName1;
   var expScanType = "ixscan";
   checkExplain( cl, cond, expIndexName, expScanType );
}
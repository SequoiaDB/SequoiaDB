/******************************************************************************
@Description : seqDB-13753:指定多个字段排序查询（包括不带索引、带索引）
@Modify list :
               2015-01-17 pusheng Ding  Init
******************************************************************************/
testConf.clName = COMMCLNAME + "_cl_13753";
main( test );

function test ( testPara )
{
   var recs = [
      { a: 3, b: 3, c: "test multi-columns sort" },
      { a: 1, b: 1, c: "sequoiadb soft com" },
      { a: 4, b: 4, c: "date" },
      { a: 0, b: 1, c: "agree sunday" },
      { a: 5, b: 4, c: "timestamp" },
      { a: 2, b: 1, c: "work-day" }];
   testPara.testCL.insert( recs );

   // 不走索引
   var cursor = testPara.testCL.find( {}, { "_id": { "$include": 0 } } ).sort( { b: 1, c: 1 } );
   var expRecs = [
      { a: 0, b: 1, c: "agree sunday" },
      { a: 1, b: 1, c: "sequoiadb soft com" },
      { a: 2, b: 1, c: "work-day" },
      { a: 3, b: 3, c: "test multi-columns sort" },
      { a: 4, b: 4, c: "date" },
      { a: 5, b: 4, c: "timestamp" }];
   commCompareResults( cursor, expRecs );

   // 走索引
   testPara.testCL.createIndex( "idx", { b: -1, c: -1 } );
   var cursor = testPara.testCL.find( {}, { "_id": { "$include": 0 } } ).sort( { b: 1, c: 1 } ).hint( { "": "idx" } );
   commCompareResults( cursor, expRecs );
}





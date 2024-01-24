/************************************
*@Description: seqDB-7987:upsert data match nothing use operator inc ,
               match operator use and/or/not
               operate object is array or non array, 
               operate object is not exist or exist,              
*@author:      zhaoyu
*@createdate:  2016.5.17
**************************************/
testConf.clName = COMMCLNAME + "_inc7987";
main( test );

function test ( testPara )
{
   //insert common object
   var commonDoc = { a: 1 };
   testPara.testCL.insert( commonDoc );

   //upsert common object when match nothing,use matches and
   var upsertCondition1 = { $inc: { a: 3, b: 2, c: 1 } };
   var findCondition1 = { $and: [{ a: { $et: { $decimal: "2" } } }, { b: { $all: [10, 20, 30] } }, { c: { $gt: 100 } }, { d: 1000 }] };
   testPara.testCL.upsert( upsertCondition1, findCondition1 );

   //check result
   var expRecs1 = [{ a: 1 }, { a: { $decimal: "5" }, b: [10, 20, 30], d: 1000, c: 1 }];
   checkResult( testPara.testCL, null, null, expRecs1, { a: 1 } );

   //upsert common object when match nothing,use matches or
   var upsertCondition2 = { $inc: { a: { $numberLong: "6" }, b: 7, c: 8 } };
   var findCondition2 = { $or: [{ a: { $et: { $decimal: "2" } } }, { b: { $all: [15, 25, 35] } }, { c: { $gt: 100 } }, { d: { $et: 1005 } }] };
   testPara.testCL.upsert( upsertCondition2, findCondition2 );

   //check result
   var expRecs2 = [{ a: 1 }, { a: { $decimal: "5" }, b: [10, 20, 30], d: 1000, c: 1 }, { a: 6, b: 7, c: 8 }]
   checkResult( testPara.testCL, null, null, expRecs2, { a: 1 } );

   //upsert common object when match nothing,use matches not
   var upsertCondition3 = { $inc: { a: { $decimal: "9", $precision: [10, 2] }, b: 10, c: 11 } };
   var findCondition3 = { $not: [{ a: { $ne: 10 } }] };
   testPara.testCL.upsert( upsertCondition3, findCondition3 );

   //check result
   var expRecs3 = [{ a: 1 }, { a: { $decimal: "5" }, b: [10, 20, 30], d: 1000, c: 1 }, { a: 6, b: 7, c: 8 }, { a: { $decimal: "9.00", $precision: [10, 2] }, b: 10, c: 11 }]
   checkResult( testPara.testCL, null, null, expRecs3, { a: 1 } );

   //upsert nested object when match nothing,use matches and
   var upsertCondition4 = { $inc: { "b.0": 3, "d.0.0": 15, d: 20 } };
   var findCondition4 = { $and: [{ "b.0": { $et: { $decimal: "2" } } }, { "c.1": { $all: [10] } }, { c: { $gt: 100 } }, { d: 1000 }] };
   testPara.testCL.upsert( upsertCondition4, findCondition4 );

   //check result
   var expRecs4 = [{ b: { 0: { $decimal: "5" } }, c: { 1: [10] }, d: { 0: { 0: 15 } }, d: 1020 }, { a: 1 }, { a: { $decimal: "5" }, b: [10, 20, 30], d: 1000, c: 1 }, { a: 6, b: 7, c: 8 }, { a: { $decimal: "9.00", $precision: [10, 2] }, b: 10, c: 11 }];
   checkResult( testPara.testCL, null, null, expRecs4, { a: 1 } );
}

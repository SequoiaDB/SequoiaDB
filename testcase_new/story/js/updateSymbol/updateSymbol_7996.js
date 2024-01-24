/************************************
*@Description: seqDB-7996:匹配不到记录，upsert使用addtoset更新符更新数组对象            
*@author:      zhaoyu
*@createdate:  2016.5.17
**************************************/

testConf.clName = COMMCLNAME + "_addtoset7996";
main( test );

function test ( testPara )
{
   //insert object
   var doc = { a: 1 };
   testPara.testCL.insert( doc );

   //upsert any object when match nothing,use matches and
   var upsertCondition1 = {
      $addtoset: {
         arr1: [100, 105, 99, 20],
         "arr2.1": [15, 10, 35],
         "arr3.1": [55, 40, 60]
      }
   };
   var findCondition1 = {
      $and: [{ "arr2.1": { $et: { $decimal: "2" } } },
      { arr1: { $all: [10, 30, 20, 10] } },
      { c: { $gt: 100 } },
      { d: 1000 },
      { "e.name.firstName": "han" },
      { arr3: [55, [35, 25, 40], 50, 45, 70] }]
   };
   testPara.testCL.upsert( upsertCondition1, findCondition1 );

   //check result
   var expRecs1 = [{
      arr1: [10, 30, 20, 10, 99, 100, 105],
      arr2: { 1: { $decimal: "2" } },
      d: 1000,
      e: { name: { firstName: "han" } },
      arr3: [55, [35, 25, 40, 55, 60], 50, 45, 70]
   },
   { a: 1 }];
   checkResult( testPara.testCL, null, null, expRecs1, { a: 1 } );

   //upsert any object when match nothing,use matches or
   var upsertCondition2 = {
      $addtoset: {
         arr1: [100, 105, 99, 20],
         "arr2.1": [15, 10, 35],
         "arr3.1": [55, 40, 60]
      }
   };
   var findCondition2 = {
      $or: [{ "arr2.1": { $et: { $decimal: "5" } } },
      { arr1: { $all: [1000, 30, 20, 10] } },
      { c: { $gt: 100 } },
      { d: 1001 },
      { "e.name.lastName": "meimei" },
      { arr4: [55, [35, 25, 40], 50, 45, 70] }]
   };
   testPara.testCL.upsert( upsertCondition2, findCondition2 );

   //check result
   var expRecs2 = [{
      arr1: [10, 30, 20, 10, 99, 100, 105],
      arr2: { 1: { $decimal: "2" } },
      d: 1000,
      e: { name: { firstName: "han" } },
      arr3: [55, [35, 25, 40, 55, 60], 50, 45, 70]
   },
   {
      arr1: [20, 99, 100, 105],
      arr2: { 1: [10, 15, 35] },
      arr3: { 1: [40, 55, 60] }
   },
   { a: 1 }];
   checkResult( testPara.testCL, null, null, expRecs2, { a: 1 } );

   //delete all data
   testPara.testCL.remove();

   //upsert any object when match nothing,use matches not
   var upsertCondition3 = {
      $addtoset: {
         arr1: [100, 105, 99, 20],
         "arr2.1": [15, 10, 35],
         "arr3.1": [55, 40, 60]
      }
   };
   var findCondition3 = {
      $not: [{ "arr2.1": { $et: { $decimal: "5" } } },
      { arr1: { $all: [1000, 30, 20, 10] } },
      { c: { $gt: 100 } },
      { d: 1001 },
      { "e.name.lastName": "meimei" },
      { arr4: [55, [35, 25, 40], 50, 45, 70] }]
   };
   testPara.testCL.upsert( upsertCondition3, findCondition3 );

   //check result
   var expRecs3 = [{
      arr1: [20, 99, 100, 105],
      arr2: { 1: [10, 15, 35] },
      arr3: { 1: [40, 55, 60] }
   }];
   checkResult( testPara.testCL, null, null, expRecs3, { a: 1 } );
}
